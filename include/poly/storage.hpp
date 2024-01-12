#ifndef POLY_STRORAGE_HPP
#define POLY_STRORAGE_HPP
#include "poly/traits.hpp"
#include <iostream>
#include <memory>
#include <new>

namespace poly {
template <bool Copyable, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class basic_sbo_storage;
class ref_storage;
template <bool Copyable, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class basic_local_storage;
template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_storage;
template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_move_only_storage;

namespace detail {
template <typename T> inline constexpr bool is_local_storage = false;

template <bool Copyable, std::size_t Size, std::size_t Alignment>
inline constexpr bool
    is_local_storage<basic_local_storage<Copyable, Size, Alignment>> = true;
template <std::size_t Size, std::size_t Alignment>
inline constexpr bool is_local_storage<local_storage<Size, Alignment>> = true;
template <std::size_t Size, std::size_t Alignment>
inline constexpr bool
    is_local_storage<local_move_only_storage<Size, Alignment>> = true;

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

  constexpr void reset() noexcept { ref_ = nullptr; }

private:
  void *ref_{nullptr};
};

static_assert(poly::traits::is_storage_v<ref_storage>);

template <bool Copyable, std::size_t Size, std::size_t Alignment>
class basic_local_storage {
public:
  constexpr basic_local_storage() {}
  basic_local_storage(ref_storage) = delete;

  /// construct with a T
  template <typename T, typename = std::enable_if_t<
                            not(poly::traits::is_storage_v<std::decay_t<T>> or
                                detail::is_local_storage<std::decay_t<T>>)>>
  basic_local_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  /// copy ctor
  template <std::size_t S, std::size_t A, bool C = Copyable,
            typename = std::enable_if_t<C>>
  basic_local_storage(const basic_local_storage<C, S, A> &other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    static_assert(C);
    if (other.vtbl_) {
      other.vtbl_->copy(buffer_, other.buffer_);
      vtbl_ = other.vtbl_;
    }
  }

  /// copy ctor
  basic_local_storage(const basic_local_storage &other) {
    if constexpr (Copyable) {
      if (other.vtbl_) {
        other.vtbl_->copy(buffer_, other.buffer_);
        vtbl_ = other.vtbl_;
      }
    }
  }

  /// move ctor
  template <std::size_t S, std::size_t A>
  basic_local_storage(basic_local_storage<Copyable, S, A> &&other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    if (other.vtbl_) {
      other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
      vtbl_ = other.vtbl_;
      other.reset();
    }
  }

  /// copy assignment
  template <std::size_t S, std::size_t A, bool C = Copyable,
            typename = std::enable_if_t<C>>
  basic_local_storage &operator=(const basic_local_storage<C, S, A> &other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    if (this == &other)
      return *this;
    reset();
    if (other.vtbl_ == nullptr)
      return *this;
    other.vtbl_->copy(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    return *this;
  }
  basic_local_storage &operator=(const basic_local_storage &other) {
    if constexpr (Copyable) {
      if (this == &other)
        return *this;
      reset();
      if (other.vtbl_ == nullptr)
        return *this;
      other.vtbl_->copy(std::addressof(buffer_), std::addressof(other.buffer_));
      vtbl_ = other.vtbl_;
    }
    return *this;
  }

  /// move assignment
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage &
  operator=(basic_local_storage<Copyable, S, A> &&other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    if (this == &other)
      return *this;
    reset();
    if (other.vtbl_ == nullptr)
      return *this;
    other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    other.reset();
    return *this;
  }
  constexpr basic_local_storage &operator=(basic_local_storage &&other) {
    if (this == &other)
      return *this;
    reset();
    if (other.vtbl_ == nullptr)
      return *this;
    other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    other.reset();
    return *this;
  }
#if __cplusplus > 201703L
  constexpr ~basic_local_storage() { reset(); }
#else
  ~basic_local_storage() { reset(); }
#endif

  /// construct a T with arguments args...
  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    static_assert(sizeof(T) <= Size, "T is to large to fit into this");
    static_assert(alignof(T) <= Alignment,
                  "The alignment of T is to large to fit into this");
    reset();
    T *ret = new (std::addressof(buffer_)) T(std::forward<Args>(args)...);
    vtbl_ = std::addressof(detail::resource_table_for<Copyable, T>);
    return *ret;
  }

  /// destroy the contained object
  constexpr void reset() {
    if (vtbl_) {
      vtbl_->destroy(std::addressof(buffer_));
      vtbl_ = nullptr;
    }
  }

  /// get pointer to contained object, or nullptr if no object is contained.
  constexpr void *data() noexcept { return vtbl_ ? this->as<void>() : nullptr; }

  /// get pointer to contained object, or nullptr if no object is contained.
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

  const detail::resource_table<Copyable> *vtbl_{nullptr};

  alignas(Alignment) std::byte buffer_[Size]{};
};

/// owing, copyable storage without dynamic allocation.
template <std::size_t Size, std::size_t Alignment>
class local_storage : public basic_local_storage<true, Size, Alignment> {
public:
  using Base = basic_local_storage<true, Size, Alignment>;
  using Base::Base;
  local_storage(local_storage &&s) : Base(std::move(s)) {}
  local_storage(const local_storage &s) : Base(s) {}
  local_storage &operator=(local_storage &&s) {
    Base::operator=(std::move(s));
    return *this;
  }
  local_storage &operator=(const local_storage &s) {
    Base::operator=(s);
    return *this;
  }
};

/// owing, non copyable storage without dynamic allocation.
template <std::size_t Size, std::size_t Alignment>
class local_move_only_storage
    : public basic_local_storage<false, Size, Alignment> {
public:
  using Base = basic_local_storage<false, Size, Alignment>;
  using Base::Base;
  using Base::data;
  using Base::emplace;
  local_move_only_storage(local_move_only_storage &&s) : Base(std::move(s)) {}
  local_move_only_storage(const local_move_only_storage &s) = delete;
  local_move_only_storage &operator=(local_move_only_storage &&s) {
    Base::operator=(std::move(s));
    return *this;
  }
  local_move_only_storage &operator=(const local_move_only_storage &s) = delete;
};

static_assert(poly::traits::is_storage_v<local_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<local_storage<64, 4>>);
static_assert(poly::traits::is_storage_v<local_move_only_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<local_move_only_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<local_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<local_storage<32, 8>>);
static_assert(std::is_move_constructible_v<local_storage<32, 8>>);
static_assert(std::is_move_assignable_v<local_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<local_move_only_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<local_move_only_storage<32, 8>>);
static_assert(std::is_move_constructible_v<local_move_only_storage<32, 8>>);
static_assert(std::is_move_assignable_v<local_move_only_storage<32, 8>>);

template <bool Copyable, std::size_t Size, std::size_t Alignment>
class basic_sbo_storage {
public:
  basic_sbo_storage() = default;
  basic_sbo_storage(ref_storage) = delete;

  template <typename T, typename = std::enable_if_t<
                            not poly::traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_sbo_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->template emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  /// move ctor
  template <size_t S, size_t A>
  constexpr basic_sbo_storage(basic_sbo_storage<Copyable, S, A> &&other) {
    if (not other.contains_value()) {
      // nothing to do
      return;
    }

    if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
      // others object fits into this small buffer
      if (other.is_heap_allocated()) {
        // move others heap object this buffer
        other.vtbl_->move(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // move others buffer object into this buffer
        other.vtbl_->move(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      }
    } else {
      if (other.is_heap_allocated()) {
        buffer.heap = other.buffer.heap;
      } else {
        buffer.heap = other.vtbl_->heap_move(std::addressof(other.buffer));
      }
      vtbl_ = other.vtbl_;
      other.buffer.heap = nullptr;
      other.vtbl_ = nullptr;
    }
  }

  /// move assignment
  template <size_t S, size_t A>
  constexpr basic_sbo_storage &
  operator=(basic_sbo_storage<Copyable, S, A> &&other) {
    if (&other == this or not other.contains_value()) {
      // nothing to do
      return *this;
    }

    reset();

    if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
      // others object fits into this small buffer
      if (other.is_heap_allocated()) {
        // move others heap object this buffer
        other.vtbl_->move(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // move others buffer object into this buffer
        other.vtbl_->move(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      }
    } else {
      if (other.is_heap_allocated()) {
        buffer.heap = other.buffer.heap;
      } else {
        buffer.heap = other.vtbl_->heap_move(std::addressof(other.buffer));
      }
      vtbl_ = other.vtbl_;
      other.buffer.heap = nullptr;
      other.vtbl_ = nullptr;
    }
    return *this;
  }

  /// copy ctor for copyable sbo storage
  template <size_t S, size_t A, bool C = Copyable,
            typename = std::enable_if_t<C>>
  constexpr basic_sbo_storage(const basic_sbo_storage<C, S, A> &other) {
    if (not other.contains_value())
      return;

    if (other.is_heap_allocated()) {
      // contained object dynamically allocated
      if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
        // others object fits into this small buffer->copy others heap object
        // into this buffer
        other.vtbl_->copy(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // others object does not fit into small buffer-> copy others heap
        // object
        buffer.heap = other.vtbl_->heap_copy(other.buffer.heap);
      }
    } else {
      // contained object allocated in buffer
      if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
        // others object fits into this small buffer->copy others buffer object
        // into this buffer
        other.vtbl_->copy(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      } else {
        // others object does not fit into small buffer-> copy others heap
        // object
        buffer.heap = other.vtbl_->heap_copy(std::addressof(other.buffer));
      }
    }
    vtbl_ = other.vtbl_;
  }

  template <size_t S, size_t A, bool C = Copyable,
            typename = std::enable_if_t<C>>
  constexpr basic_sbo_storage &
  operator=(const basic_sbo_storage<C, S, A> &other) {
    if (&other == this or not other.contains_value())
      return *this;

    reset();

    if (other.is_heap_allocated()) {
      if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
        // others object fits into this small buffer->move others heap object
        // into this buffer
        other.vtbl_->move(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // others object does not fit into small buffer-> take over pointer
        buffer.heap = other.buffer.heap;
        other.buffer.heap = nullptr;
        other.vtbl_ = nullptr;
      }
    } else {
      if (other.vtbl_->size <= S and other.vtbl_->align <= Alignment) {
        // others object fits into this small buffer->move others buffer object
        // into this buffer
        other.vtbl_->move(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      } else {
        // others object does not fit into small buffer-> take over pointer
        buffer.heap = other.vtbl_->heap_move(std::addressof(other.buffer));
        other.buffer.heap = nullptr;
        other.vtbl_ = nullptr;
      }
    }
    vtbl_ = other.vtbl_;
    return *this;
  }
#if __cplusplus > 201703L
  constexpr ~basic_sbo_storage() { reset(); }
#else
  ~basic_sbo_storage() { reset(); }
#endif

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    reset();
    T *ret = nullptr;
    if constexpr (sizeof(T) <= Size and alignof(T) <= Alignment) {
      ret = new (std::addressof(buffer.buffer)) T(std::forward<Args>(args)...);
      vtbl_ = std::addressof(detail::sbo_table_for<false, T>);
    } else {
      ret = new T(std::forward<Args>(args)...);
      buffer.heap = ret;
    }
    return *ret;
  }

  constexpr void reset() {
    if (not this->contains_value())
      return;

    if (vtbl_->size > Size or vtbl_->align > Alignment)
      vtbl_->heap_destroy(buffer.heap);
    else
      vtbl_->destroy(std::addressof(buffer.buffer));
    vtbl_ = nullptr;
  }

  constexpr void *data() noexcept {
    return this->contains_value() ? this->as<void>() : nullptr;
  }

  constexpr const void *data() const noexcept {
    return this->contains_value() ? this->as<const void>() : nullptr;
  }

private:
  constexpr bool contains_value() const noexcept { return vtbl_ != nullptr; }

  constexpr bool is_heap_allocated() const noexcept {
    if (not vtbl_)
      return false;
    if (vtbl_->size > Size or vtbl_->align > Alignment)
      return true;
    return false;
  }

  template <typename T> constexpr T *as() noexcept {
    if (vtbl_->size <= Size and vtbl_->align <= Alignment)
      return static_cast<T *>(
          static_cast<void *>(std::addressof(buffer.buffer)));
    return static_cast<T *>(buffer.heap);
  }

  template <typename T> constexpr const T *as() const noexcept {
    if (vtbl_->size <= Size and vtbl_->align <= Alignment)
      return static_cast<const T *>(
          static_cast<const void *>(std::addressof(buffer.buffer)));
    return static_cast<const T *>(buffer.heap);
  }

  const detail::sbo_resource_table<false> *vtbl_{nullptr};
  detail::raw_sbo_storage<Size, Alignment> buffer;
};

template <std::size_t Size, std::size_t Alignment>
class sbo_storage : public basic_sbo_storage<true, Size, Alignment> {
public:
  using Base = basic_sbo_storage<true, Size, Alignment>;
  using Base::Base;
  using Base::operator=;
  using Base::data;
  using Base::emplace;
};

template <std::size_t Size, std::size_t Alignment>
class sbo_move_only_storage : public basic_sbo_storage<false, Size, Alignment> {
public:
  using Base = basic_sbo_storage<false, Size, Alignment>;
  using Base::Base;
  using Base::operator=;
  using Base::data;
  using Base::emplace;
  sbo_move_only_storage(const sbo_move_only_storage &) = delete;
  sbo_move_only_storage &operator=(const sbo_move_only_storage &) = delete;
};

static_assert(poly::traits::is_storage_v<sbo_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<sbo_storage<64, 4>>);
// static_assert(poly::traits::is_storage_v<sbo_move_only_storage<32, 8>>);
// static_assert(poly::traits::is_storage_v<sbo_move_only_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<sbo_storage<32, 8>>);
static_assert(std::is_move_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_move_assignable_v<sbo_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<sbo_move_only_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<sbo_move_only_storage<32, 8>>);
// static_assert(std::is_move_constructible_v<sbo_move_only_storage<32, 8>>);
// static_assert(std::is_move_assignable_v<sbo_move_only_storage<32, 8>>);
} // namespace poly
#endif
