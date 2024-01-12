#ifndef POLY_STRORAGE_HPP
#define POLY_STRORAGE_HPP
#include "poly/traits.hpp"
#include <memory>
#include <new>

namespace poly {
namespace detail {

template <bool Copyable> struct resource_table {
  void (*copy)(void *dest, const void *src);
  void (*move)(void *dest, void *src);
  void (*destroy)(void *dest);
};

template <> struct resource_table<false> {
  void (*move)(void *dest, void *src);
  void (*destroy)(void *dest);
};

template <std::size_t Size, std::size_t Align> union raw_sbo_storage {
  void *heap;
  alignas(Align) std::byte buffer[Size];
};

template <bool Copyable> struct sbo_resource_table {
  void (*copy)(void *dest,
               const void *src); ///< copy from one local buffer to another
  void *(*heap_copy)(const void *src); ///< allocates new heap copy
  void (*move)(void *dest,
               void *src); ///< move from one local buffer to another
  void *(*heap_move)(
      void *src); ///< allocates new heap copy move constructed from src
  void (*destroy)(void *dest);      ///< destroy object in local buffer
  void (*heap_destroy)(void *dest); ///< destroy object on heap
  size_t size;
  size_t align;
};
template <> struct sbo_resource_table<false> {
  void (*move)(void *dest,
               void *src); ///< move from one local buffer to another
  void *(*heap_move)(
      void *src); ///< allocates new heap copy move constructed from src
  void (*destroy)(void *dest);      ///< destroy object in local buffer
  void (*heap_destroy)(void *dest); ///< destroy object on heap
  size_t size;
  size_t align;
};
template <bool Copyable, typename T>
constexpr resource_table<Copyable> get_local_resource_table() noexcept {
  if constexpr (Copyable) {
    return resource_table<true>{
        .copy =
            +[](void *dest, const void *src) {
              new (dest) T(*static_cast<const T *>(src));
            },
        .move =
            +[](void *dest, void *src) {
              new (dest) T(std::move(*static_cast<T *>(src)));
            },
        .destroy = +[](void *src) { static_cast<T *>(src)->~T(); }};
  } else {
    return resource_table<false>{
        .move =
            +[](void *dest, void *src) {
              new (dest) T(std::move(*static_cast<T *>(src)));
            },
        .destroy = +[](void *src) { static_cast<T *>(src)->~T(); }};
  }
}

template <typename T, size_t Size, size_t Align>
inline constexpr bool fits_into_small_buffer_v =
    sizeof(T) <= Size && alignof(T) <= Align;

template <bool Copyable, typename T>
constexpr sbo_resource_table<Copyable> get_sbo_resource_table() noexcept {
  if constexpr (Copyable) {
    return sbo_resource_table<true>{
        .copy =
            +[](void *dest, const void *src) {
              new (dest) T(*static_cast<const T *>(src));
            },
        .heap_copy = +[](const void *src) -> void * {
          return new T(static_cast<const T *>(src));
        },
        .move =
            +[](void *dest, void *src) {
              new (dest) T(std::move(*static_cast<T *>(src)));
            },
        .heap_move = +[](void *src) -> void * {
          return new T(std::move(*static_cast<T *>(src)));
        },
        .destroy = +[](void *src) { static_cast<T *>(src)->~T(); },
        .heap_destroy = +[](void *src) { delete static_cast<T *>(src); },
        .size = sizeof(T),
        .align = alignof(T)};
  } else {
    return sbo_resource_table<false>{
        .move =
            +[](void *dest, void *src) {
              new (dest) T(std::move(*static_cast<T *>(src)));
            },
        .heap_move = +[](void *src) -> void * {
          return new T(std::move(*static_cast<T *>(src)));
        },
        .destroy = +[](void *src) { static_cast<T *>(src)->~T(); },
        .heap_destroy = +[](void *src) { delete static_cast<T *>(src); },
        .size = sizeof(T),
        .align = alignof(T)};
  }
}

template <bool Copyable, typename T>
inline constexpr resource_table<Copyable> resource_table_for =
    get_local_resource_table<Copyable, T>();

template <bool Copyable, typename T>
inline constexpr sbo_resource_table<Copyable> sbo_table_for =
    get_sbo_resource_table<Copyable, T>();
} // namespace detail

/// non owing storage. Only contains pointer to object emplaced.
class ref_storage {
public:
  template <typename T,
            typename = std::enable_if_t<not(std::is_same_v<ref_storage, T> or
                                            poly::traits::is_storage_v<T>)>>
  constexpr ref_storage(T &t) noexcept : ref_{std::addressof(t)} {}

  template <POLY_STORAGE Storage, typename S = Storage,
            typename = std::enable_if_t<poly::traits::is_storage_v<S>>>
  constexpr ref_storage(Storage &s) noexcept : ref_{s.data()} {}
  constexpr ref_storage() noexcept = default;
  constexpr ref_storage(const ref_storage &) noexcept = default;
  constexpr ref_storage(ref_storage &&) noexcept = default;
  constexpr ref_storage &operator=(const ref_storage &) noexcept = default;
  constexpr ref_storage &operator=(ref_storage &&) noexcept = default;
  template <typename T> constexpr T &emplace(T &t) noexcept {
    ref_ = std::addressof(t);
    return t;
  }
  constexpr void *data() noexcept { return ref_; }
  constexpr const void *data() const noexcept { return ref_; }

  constexpr void reset() { ref_ = nullptr; }

private:
  void *ref_{nullptr};
};

static_assert(poly::traits::is_storage_v<ref_storage>);

/// owing, copyable storage without dynamic allocation.
template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_storage {
public:
  local_storage() = default;
  local_storage(ref_storage) = delete;
  template <typename T, typename = std::enable_if_t<
                            not std::is_same_v<local_storage, std::decay_t<T>>>>
  constexpr local_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  constexpr local_storage(const local_storage &other) {
    if (other.vtbl_) {
      other.vtbl_->copy(buffer_, other.buffer_);
      vtbl_ = other.vtbl_;
    }
  }

  constexpr local_storage(local_storage &&other) {
    if (other.vtbl_) {
      other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
      vtbl_ = other.vtbl_;
    }
  }

  constexpr local_storage &operator=(const local_storage &other) {
    if (this == &other)
      return *this;
    reset();
    other.vtbl_->copy(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    return *this;
  }

  constexpr local_storage &operator=(local_storage &&other) {
    if (this == &other)
      return *this;
    reset();
    other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    return *this;
  }

#if __cplusplus > 201703L
  constexpr ~local_storage() { reset(); }
#else
  ~local_storage() { reset(); }
#endif

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    static_assert(sizeof(T) <= Size, "T is to large to fit into local_storage");
    static_assert(alignof(T) <= Alignment,
                  "The alignment of T is to large to fit into local_storage");
    reset();
    T *ret = new (std::addressof(buffer_)) T(std::forward<Args>(args)...);
    vtbl_ = std::addressof(detail::resource_table_for<true, T>);
    return *ret;
  }

  constexpr void reset() noexcept {
    if (vtbl_) {
      vtbl_->destroy(std::addressof(buffer_));
      vtbl_ = nullptr;
    }
  }

  constexpr void *data() noexcept { return vtbl_ ? this->as<void>() : nullptr; }

  constexpr const void *data() const noexcept {
    return vtbl_ ? this->as<const void>() : nullptr;
  }

private:
  template <typename T> constexpr T *as() noexcept {
    return static_cast<T *>(static_cast<void *>(std::addressof(buffer_)));
  }

  template <typename T> constexpr const T *as() const noexcept {
    return static_cast<const T *>(
        static_cast<const void *>(std::addressof(buffer_)));
  }

  const detail::resource_table<true> *vtbl_{nullptr};

  alignas(Alignment) std::byte buffer_[Size];
};
static_assert(poly::traits::is_storage_v<local_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<local_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<local_storage<64, 4>>);
static_assert(poly::traits::is_storage_v<local_storage<64, 4>>);

/// owing, non copyable storage without dynamic allocation.
template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_move_only_storage {
public:
  local_move_only_storage() = default;
  local_move_only_storage(ref_storage) = delete;
  local_move_only_storage(const local_move_only_storage &other) = delete;
  local_move_only_storage &
  operator=(const local_move_only_storage &other) = delete;

  template <typename T, typename = std::enable_if_t<not std::is_same_v<
                            local_move_only_storage, std::decay_t<T>>>>
  constexpr local_move_only_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->template emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  constexpr local_move_only_storage(local_move_only_storage &&other) {
    if (other.vtbl_) {
      other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
      vtbl_ = other.vtbl_;
    }
  }

  constexpr local_move_only_storage &
  operator=(local_move_only_storage &&other) {
    if (this == &other)
      return *this;
    reset();
    other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
    this->vtbl_ = other.vtbl_;
    return *this;
  }

#if __cplusplus > 201703L
  constexpr ~local_move_only_storage() { reset(); }
#else
  ~local_move_only_storage() { reset(); }
#endif

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    static_assert(sizeof(T) <= Size, "T is to large to fit into local_storage");
    static_assert(alignof(T) <= Alignment,
                  "The alignment of T is to large to fit into local_storage");
    reset();
    T *ret = new (std::addressof(buffer_)) T(std::forward<Args>(args)...);
    vtbl_ = std::addressof(detail::resource_table_for<false, T>);
    return *ret;
  }

  constexpr void reset() noexcept {
    if (vtbl_) {
      vtbl_->destroy(std::addressof(buffer_));
      vtbl_ = nullptr;
    }
  }

  constexpr void *data() noexcept { return vtbl_ ? this->as<void>() : nullptr; }

  constexpr const void *data() const noexcept {
    return vtbl_ ? this->as<const void>() : nullptr;
  }

private:
  template <typename T> constexpr T *as() noexcept {
    return static_cast<T *>(static_cast<void *>(std::addressof(buffer_)));
  }

  template <typename T> constexpr const T *as() const noexcept {
    return static_cast<const T *>(
        static_cast<const void *>(std::addressof(buffer_)));
  }

  const detail::resource_table<false> *vtbl_{nullptr};
  alignas(Alignment) std::byte buffer_[Size];
};

template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class sbo_storage {
public:
  sbo_storage() = default;
  sbo_storage(ref_storage) = delete;
  sbo_storage(const sbo_storage &other) = delete;
  sbo_storage &operator=(const sbo_storage &other) = delete;

  template <typename T, typename = std::enable_if_t<
                            not std::is_same_v<sbo_storage, std::decay_t<T>>>>
  constexpr sbo_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->template emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  constexpr sbo_storage(sbo_storage &&other) {
    if (other.vtbl_) {
      other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
      vtbl_ = other.vtbl_;
    }
  }

  constexpr sbo_storage &operator=(sbo_storage &&other) {
    if (this == &other)
      return *this;
    reset();
    if (other.vtbl_) {
      other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
      this->vtbl_ = other.vtbl_;
    }
    return *this;
  }

#if __cplusplus > 201703L
  constexpr ~sbo_storage() { reset(); }
#else
  ~sbo_storage() { reset(); }
#endif

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    static_assert(sizeof(T) <= Size, "T is to large to fit into local_storage");
    static_assert(alignof(T) <= Alignment,
                  "The alignment of T is to large to fit into local_storage");
    reset();
    T *ret = nullptr;
    if constexpr (sizeof(T) <= Size and alignof(T) <= Alignment) {
      ret = new (std::addressof(buffer.buffer)) T(std::forward<Args>(args)...);
      vtbl_ = std::addressof(detail::sbo_table_for<true, T>);
    } else {
      ret = new T(std::forward<Args>(args)...);
      buffer.heap = ret;
    }
    return *ret;
  }

  constexpr void reset() {
    if (vtbl_) {
      if (vtbl_->size > Size)
        vtbl_->heap_destroy(std::addressof(buffer.heap));
      else
        vtbl_->destroy(std::addressof(buffer.buffer));
      vtbl_ = nullptr;
    }
  }

  constexpr void *data() noexcept { return vtbl_ ? this->as<void>() : nullptr; }

  constexpr const void *data() const noexcept {
    return vtbl_ ? this->as<const void>() : nullptr;
  }

private:
  template <typename T> constexpr T *as() noexcept {
    return static_cast<T *>(static_cast<void *>(std::addressof(buffer_)));
  }

  template <typename T> constexpr const T *as() const noexcept {
    return static_cast<const T *>(
        static_cast<const void *>(std::addressof(buffer_)));
  }

  const detail::sbo_resource_table<false> *vtbl_{nullptr};
  detail::raw_sbo_storage<Size, Alignment> buffer;
};
} // namespace poly
#endif
