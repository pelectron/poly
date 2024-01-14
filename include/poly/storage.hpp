#ifndef POLY_STRORAGE_HPP
#define POLY_STRORAGE_HPP
#include "poly/traits.hpp"
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
class move_only_local_storage;

namespace detail {
template <typename T> inline constexpr bool is_local_storage = false;

template <bool Copyable, std::size_t Size, std::size_t Alignment>
inline constexpr bool
    is_local_storage<basic_local_storage<Copyable, Size, Alignment>> = true;
template <std::size_t Size, std::size_t Alignment>
inline constexpr bool is_local_storage<local_storage<Size, Alignment>> = true;
template <std::size_t Size, std::size_t Alignment>
inline constexpr bool
    is_local_storage<move_only_local_storage<Size, Alignment>> = true;

/// table of function pointers for resource managment used by local_storage
template <bool Copyable> struct resource_table {
  void (*copy)(void *dest, const void *src);
  void (*move)(void *dest, void *src);
  void (*destroy)(void *dest);
};

/// table of function pointers for resource managment used by
/// local_move_only_storage
template <> struct resource_table<false> {
  void (*move)(void *dest, void *src);
  void (*destroy)(void *dest);
};

/// raw storage type for small buffer optimized storage
/// contains of either a pointer to the heap or the object in the local buffer
template <std::size_t Size, std::size_t Align> union raw_sbo_storage {
  void *heap;
  alignas(Align) std::byte buffer[Size];
};

/// table of function pointers for resource managment used by sbo_storage
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

/// table of function pointers for resource managment used by
/// sbo_move_only_storage
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

/// returns a fully populated resource_table
template <bool Copyable, typename T>
constexpr resource_table<Copyable> get_local_resource_table() noexcept {
  if constexpr (Copyable) {
    return resource_table<true>{
        //.copy =
        +[](void *dest, const void *src) {
          ::new (dest) T(*static_cast<const T *>(src));
        },
        //.move =
        +[](void *dest, void *src) {
          ::new (dest) T(std::move(*static_cast<T *>(src)));
        },
        //.destroy =
        +[](void *src) { static_cast<T *>(src)->~T(); }};
  } else {
    return resource_table<false>{
        //.move =
        +[](void *dest, void *src) {
          ::new (dest) T(std::move(*static_cast<T *>(src)));
        },
        //.destroy =
        +[](void *src) { static_cast<T *>(src)->~T(); }};
  }
}

/// returns a fully populated sbo_resource_table
template <bool Copyable, typename T>
constexpr sbo_resource_table<Copyable> get_sbo_resource_table() noexcept {
  if constexpr (Copyable) {
    return sbo_resource_table<Copyable>{
        // .copy =
        +[](void *dest, const void *src) {
          ::new (dest) T(*static_cast<const T *>(src));
        },
        // .heap_copy =
        +[](const void *src) -> void * {
          return ::new T(*static_cast<const T *>(src));
        },
        // .move =
        +[](void *dest, void *src) {
          ::new (dest) T(std::move(*static_cast<T *>(src)));
        },
        // .heap_move =
        +[](void *src) -> void * {
          return ::new T(std::move(*static_cast<T *>(src)));
        },
        // .destroy =
        +[](void *src) { static_cast<T *>(src)->~T(); },
        // .heap_destroy =
        +[](void *src) { ::delete static_cast<T *>(src); },
        // .size =
        sizeof(T),
        // .align =
        alignof(T)};
  } else {
    return sbo_resource_table<Copyable>{
        // .move =
        +[](void *dest, void *src) {
          ::new (dest) T(std::move(*static_cast<T *>(src)));
        },
        // .heap_move =
        +[](void *src) -> void * {
          return ::new T(std::move(*static_cast<T *>(src)));
        },
        // .destroy =
        +[](void *src) { static_cast<T *>(src)->~T(); },
        // .heap_destroy =
        +[](void *src) { ::delete static_cast<T *>(src); },
        // .size =
        sizeof(T),
        // .align =
        alignof(T)};
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
class ref_storage final {
public:
  template <typename T,
            typename = std::enable_if_t<not(poly::traits::is_storage_v<T>)>>
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

/// local storage implementation.
///
/// Holds object emplaced in a buffer of Size bytes with an alignement of
/// Alignment. Objects greater than Size or stricter alignment than Alignment
/// cannot be emplaced.
/// @tparam Copyable specify if the storage is copyable
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <bool Copyable, std::size_t Size, std::size_t Alignment>
class basic_local_storage {
public:
  template <bool C, std::size_t S, std::size_t A>
  friend class basic_local_storage;

  constexpr basic_local_storage() noexcept {}

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
  template <std::size_t S, std::size_t A, bool C = Copyable>
  constexpr basic_local_storage(const basic_local_storage<C, S, A> &other) {
    static_assert(Copyable);
    this->copy(other);
  }
  /// copy ctor
  constexpr basic_local_storage(const basic_local_storage &other) {
    /// this definition is needed, else the compiler produces a memcpy for the
    /// copy ctor instead of choosing the template version
    static_assert(Copyable);
    this->copy(other);
  }

  /// move ctor
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage(basic_local_storage<Copyable, S, A> &&other) {
    this->move(std::move(other));
  }

  /// copy assignment
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage &
  operator=(const basic_local_storage<Copyable, S, A> &other) {
    static_assert(Copyable);
    return this->copy(other);
  }

  /// copy assignment
  constexpr basic_local_storage &operator=(const basic_local_storage &other) {
    static_assert(Copyable);
    return this->copy(other);
  }

  /// move assignment
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage &
  operator=(basic_local_storage<Copyable, S, A> &&other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    return this->move(std::move(other));
  }

  // /// move assignment
  // constexpr basic_local_storage &operator=(basic_local_storage &&other) {
  //   return this->move(std::move(other));
  // }

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
    T *ret = ::new (std::addressof(buffer_)) T(std::forward<Args>(args)...);
    vtbl_ = std::addressof(detail::resource_table_for<Copyable, T>);
    return *ret;
  }

  /// get pointer to contained object, or nullptr if no object is contained.
  constexpr void *data() noexcept { return vtbl_ ? this->as<void>() : nullptr; }

  /// get pointer to contained object, or nullptr if no object is contained.
  constexpr const void *data() const noexcept {
    return vtbl_ ? this->as<const void>() : nullptr;
  }

private:
  /// destroy the contained object
  constexpr void reset() {
    if (vtbl_) {
      vtbl_->destroy(std::addressof(buffer_));
      vtbl_ = nullptr;
    }
  }

  template <typename T> constexpr T *as() noexcept {
    return static_cast<T *>(static_cast<void *>(std::addressof(buffer_)));
  }

  template <typename T> constexpr const T *as() const noexcept {
    return static_cast<const T *>(
        static_cast<const void *>(std::addressof(buffer_)));
  }

  /// move implementation
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage &
  move(basic_local_storage<Copyable, S, A> &&other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    if constexpr (S == Size and A == Alignment) {
      // only check for this if other is of same type
      if (this == &other)
        return *this;
    }
    reset();
    if (other.vtbl_ == nullptr)
      return *this;
    other.vtbl_->move(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    other.reset();
    return *this;
  }

  /// copy implementation
  template <std::size_t S, std::size_t A>
  constexpr basic_local_storage &
  copy(const basic_local_storage<Copyable, S, A> &other) {
    static_assert(S <= Size,
                  "The local_storage to copy from is too big to fit into this");
    static_assert(A <= Alignment, "The alignemnt of the local_storage to copy "
                                  "from is too big to fit into this");
    static_assert(Copyable);
    if constexpr (S == Size and A == Alignment) {
      // only check for this if other is of same type
      if (this == &other)
        return *this;
    }
    reset();
    if (other.vtbl_ == nullptr)
      return *this;
    other.vtbl_->copy(std::addressof(buffer_), std::addressof(other.buffer_));
    vtbl_ = other.vtbl_;
    return *this;
  }

  const detail::resource_table<Copyable> *vtbl_{nullptr};

  alignas(Alignment) std::byte buffer_[Size]{};
};

/// owing, copyable storage without dynamic allocation.
///
/// Holds object emplaced in a buffer of Size bytes with an alignment of
/// Alignment. Objects greater than Size or stricter alignment than Alignment
/// cannot be emplaced.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <std::size_t Size, std::size_t Alignment>
class local_storage final : public basic_local_storage<true, Size, Alignment> {
public:
  using Base = basic_local_storage<true, Size, Alignment>;

  /// construct empty storage
  constexpr local_storage() noexcept : Base() {}

  /// construct with a T
  template <typename T, typename = std::enable_if_t<
                            not(poly::traits::is_storage_v<std::decay_t<T>>)>>
  local_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>)
      : Base(std::forward<T>(t)) {}

  /// move ctor
  local_storage(local_storage &&s) : Base(std::move(s)) {}

  /// copy ctor
  template <std::size_t S, std::size_t A>
  local_storage(const local_storage<S, A> &s) : Base(s) {}
  local_storage(const local_storage &s) : Base(s) {}

  /// move assignment
  template <std::size_t S, std::size_t A>
  local_storage &operator=(local_storage<S, A> &&s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// copy assignment
  template <std::size_t S, std::size_t A>
  local_storage &operator=(const local_storage<A, S> &s) {
    Base::operator=(s);
    return *this;
  }
  local_storage &operator=(const local_storage &s) {
    Base::operator=(s);
    return *this;
  }
};

/// owing, non copyable storage without dynamic allocation.
///
/// Holds object emplaced in a buffer of Size bytes with an alignment of
/// Alignment. Objects greater than Size or stricter alignment than Alignment
/// cannot be emplaced.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <std::size_t Size, std::size_t Alignment>
class move_only_local_storage final
    : public basic_local_storage<false, Size, Alignment> {
public:
  using Base = basic_local_storage<false, Size, Alignment>;
  using Base::data;
  using Base::emplace;

  /// construct empty storage
  constexpr move_only_local_storage() noexcept : Base() {}

  /// construct with a T
  template <typename T, typename = std::enable_if_t<
                            not(poly::traits::is_storage_v<std::decay_t<T>>)>>
  move_only_local_storage(T &&t) noexcept(
      std::is_nothrow_move_constructible_v<std::decay_t<T>>)
      : Base(std::move(t)) {}

  /// move ctor
  template<size_t S,size_t A>
  move_only_local_storage(move_only_local_storage<S,A> &&s) : Base(std::move(s)) {}

  /// deleted copy ctor
  move_only_local_storage(const move_only_local_storage &s) = delete;

  /// move assignment
  template<size_t S,size_t A>
  move_only_local_storage &operator=(move_only_local_storage<S,A> &&s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// deleted copy assignment
  move_only_local_storage &operator=(const move_only_local_storage &s) = delete;
};

static_assert(poly::traits::is_storage_v<local_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<local_storage<64, 4>>);
static_assert(poly::traits::is_storage_v<move_only_local_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<move_only_local_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<local_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<local_storage<32, 8>>);
static_assert(std::is_move_constructible_v<local_storage<32, 8>>);
static_assert(std::is_move_assignable_v<local_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<move_only_local_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<move_only_local_storage<32, 8>>);
static_assert(std::is_move_constructible_v<move_only_local_storage<32, 8>>);
static_assert(std::is_move_assignable_v<move_only_local_storage<32, 8>>);

/// storage with small buffer optimization implementation. Emplaced objects are
/// allocated within a buffer of Size with alignment Alignment if the object
/// satisfies these constraints, else it is heap allocated.
/// @tparam Copyable specify if the storage is copyable
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <bool Copyable, std::size_t Size, std::size_t Alignment>
class basic_sbo_storage {
public:
  template <bool C, std::size_t S, std::size_t A>
  friend class basic_sbo_storage;

  constexpr basic_sbo_storage() noexcept {}
  basic_sbo_storage(ref_storage) = delete;

  template <typename T, typename = std::enable_if_t<
                            not poly::traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_sbo_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>) {
    this->template emplace<std::decay_t<T>>(std::forward<T>(t));
  }

  /// copy ctor for copyable sbo storage
  template <size_t S, size_t A>
  constexpr basic_sbo_storage(const basic_sbo_storage<Copyable, S, A> &other) {
    static_assert(Copyable);
    this->copy(other);
  }

  /// copy ctor for copyable sbo storage
  constexpr basic_sbo_storage(const basic_sbo_storage &other) {
    /// this definition is needed, else the compiler produces a memcpy for the
    /// copy ctor instead of choosing the template version
    static_assert(Copyable);
    this->copy(other);
  }

  /// move ctor
  template <size_t S, size_t A>
  constexpr basic_sbo_storage(basic_sbo_storage<Copyable, S, A> &&other) {
    this->move(std::move(other));
  }

  /// move ctor
  constexpr basic_sbo_storage(basic_sbo_storage &&other) {
    this->move(std::move(other));
  }

  /// move assignment
  template <size_t S, size_t A>
  constexpr basic_sbo_storage &
  operator=(basic_sbo_storage<Copyable, S, A> &&other) {
    return this->move(std::move(other));
  }

  /// move assignment
  constexpr basic_sbo_storage &operator=(basic_sbo_storage &&other) {
    return this->move(std::move(other));
  }

  /// copy assignment
  template <size_t S, size_t A>
  constexpr basic_sbo_storage &
  operator=(const basic_sbo_storage<Copyable, S, A> &other) {
    static_assert(Copyable);
    return this->copy(other);
  }

  /// copy assignment
  constexpr basic_sbo_storage &operator=(const basic_sbo_storage &other) {
    static_assert(Copyable);
    return this->copy(other);
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
      ret =
          ::new (std::addressof(buffer.buffer)) T(std::forward<Args>(args)...);
    } else {
      ret = ::new T(std::forward<Args>(args)...);
      buffer.heap = ret;
    }
    vtbl_ = std::addressof(detail::sbo_table_for<Copyable, T>);
    return *ret;
  }

  constexpr void *data() noexcept {
    return this->contains_value() ? this->as<void>() : nullptr;
  }

  constexpr const void *data() const noexcept {
    return this->contains_value() ? this->as<const void>() : nullptr;
  }

private:
  constexpr void reset() {
    if (not this->contains_value())
      return;

    if (vtbl_->size > Size or vtbl_->align > Alignment)
      vtbl_->heap_destroy(buffer.heap);
    else
      vtbl_->destroy(std::addressof(buffer.buffer));
    vtbl_ = nullptr;
  }

  template <size_t S, size_t A>
  constexpr basic_sbo_storage &move(basic_sbo_storage<Copyable, S, A> &&other) {
    if constexpr (S == Size and A == Alignment) {
      if (&other == this)
        return *this;
    }
    reset();
    if (not other.contains_value()) {
      // nothing to do
      return *this;
    }


    if (other.vtbl_->size <= Size and other.vtbl_->align <= Alignment) {
      // others object fits into this small buffer
      if (other.is_heap_allocated()) {
        // move others heap object this buffer
        other.vtbl_->move(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // move others buffer object into this buffer
        other.vtbl_->move(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      }
      // copy others vtable before others reset
      // others vtable is not touched to ensure proper destruction
      // of others object on heap
      vtbl_ = other.vtbl_;
    } else {
      // others object does not fit into this small buffer
      if (other.is_heap_allocated()) {
        // simply copy pointers and set others to null
        buffer.heap = other.buffer.heap;
        vtbl_ = other.vtbl_;
        other.buffer.heap = nullptr;
        other.vtbl_ = nullptr; // manual reset without dtor
      } else {
        // move object from others bufffer into heap, copy vtable.
        // others vtable is not touched to ensure proper destruction
        // of others object in buffer
        buffer.heap = other.vtbl_->heap_move(std::addressof(other.buffer));
        vtbl_ = other.vtbl_;
      }
    }
    other.reset();
    return *this;
  }
  template <size_t S, size_t A>
  constexpr basic_sbo_storage &
  copy(const basic_sbo_storage<Copyable, S, A> &other) {
    static_assert(Copyable);
    if constexpr (S == Size and A == Alignment) {
      if (&other == this)
        return *this;
    }
    reset();
    if (not other.contains_value()) {
      // nothing to do
      return *this;
    }

    if (other.vtbl_->size <= Size and other.vtbl_->align <= Alignment) {
      // others object fits into this small buffer
      if (other.is_heap_allocated()) {
        // copy others heap object into this buffer
        other.vtbl_->copy(std::addressof(buffer.buffer), other.buffer.heap);
      } else {
        // copy others buffer object into this buffer
        other.vtbl_->copy(std::addressof(buffer.buffer),
                          std::addressof(other.buffer.buffer));
      }
    } else {
      // others object does not fit into small buffer
      if (other.is_heap_allocated()) {
        // heap copy
        buffer.heap = other.vtbl_->heap_copy(other.buffer.heap);
      } else {
        // heap copy
        buffer.heap = other.vtbl_->heap_copy(std::addressof(other.buffer));
      }
    }
    vtbl_ = other.vtbl_;
    return *this;
  }

  constexpr bool contains_value() const noexcept { return vtbl_ != nullptr; }

  constexpr bool is_heap_allocated() const noexcept {
    if (not contains_value())
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

  const detail::sbo_resource_table<Copyable> *vtbl_{nullptr};
  detail::raw_sbo_storage<Size, Alignment> buffer;
};

/// Copyable storage with small buffer optimization.
///
/// Emplaced objects are allocated within a buffer of Size with alignment
/// Alignment if the object satisfies these constraints, else it is heap
/// allocated.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <std::size_t Size, std::size_t Alignment>
class sbo_storage final : public basic_sbo_storage<true, Size, Alignment> {
public:
  using Base = basic_sbo_storage<true, Size, Alignment>;
  using Base::data;
  using Base::emplace;

  /// construct empty storage
  constexpr sbo_storage() noexcept : Base() {}

  /// construct with a T
  template <typename T, typename = std::enable_if_t<
                            not(poly::traits::is_storage_v<std::decay_t<T>>)>>
  sbo_storage(T &&t) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<T>, T &&>)
      : Base(std::forward<T>(t)) {}

  /// move ctor
  template <size_t S, size_t A>
  constexpr sbo_storage(sbo_storage<S, A> &&s) : Base(std::move(s)) {}

  /// copy ctor
  sbo_storage(const sbo_storage &s) : Base(s) {}
  template <size_t S, size_t A>
  constexpr sbo_storage(const sbo_storage<S, A> &s) : Base(s) {}

  /// move assignemnt
  template <size_t S, size_t A>
  constexpr sbo_storage &operator=(sbo_storage<S, A> &&s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// copy assignemnt
  template <size_t S, size_t A>
  constexpr sbo_storage &operator=(const sbo_storage<S, A> &s) {
    Base::operator=(s);
    return *this;
  }
};

/// Move only storage with small buffer optimization.
///
/// Emplaced objects are allocated within a buffer of Size with alignment
/// Alignment if the object satisfies these constraints, else it is heap
/// allocated.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template <std::size_t Size, std::size_t Alignment>
class move_only_sbo_storage final
    : public basic_sbo_storage<false, Size, Alignment> {
public:
  using Base = basic_sbo_storage<false, Size, Alignment>;
  using Base::data;
  using Base::emplace;
  /// construct empty storage
  constexpr move_only_sbo_storage() noexcept : Base() {}

  /// construct with a T
  template <typename T, typename = std::enable_if_t<
                            not(poly::traits::is_storage_v<std::decay_t<T>>)>>
  constexpr move_only_sbo_storage(T &&t) noexcept(
      std::is_nothrow_move_constructible_v<std::decay_t<T>>)
      : Base(std::move(t)) {}

  /// move ctor
  template <size_t S, size_t A>
  constexpr move_only_sbo_storage(move_only_sbo_storage<S, A> &&s)
      : Base(std::move(s)) {}

  /// deleted copy ctor
  move_only_sbo_storage(const move_only_sbo_storage &s) = delete;

  /// move assignment
  // constexpr move_only_sbo_storage &operator=(move_only_sbo_storage &&s) {
  //   Base::operator=(std::move(s));
  //   return *this;
  // }
  template <size_t S, size_t A>
  constexpr move_only_sbo_storage &operator=(move_only_sbo_storage<S, A> &&s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// deleted copy assignment
  move_only_sbo_storage &operator=(const move_only_sbo_storage &s) = delete;
};

static_assert(poly::traits::is_storage_v<sbo_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<sbo_storage<64, 4>>);
static_assert(poly::traits::is_storage_v<move_only_sbo_storage<32, 8>>);
static_assert(poly::traits::is_storage_v<move_only_sbo_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<sbo_storage<32, 8>>);
static_assert(std::is_move_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_move_assignable_v<sbo_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<move_only_sbo_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<move_only_sbo_storage<32, 8>>);
static_assert(std::is_move_constructible_v<move_only_sbo_storage<32, 8>>);
static_assert(std::is_move_assignable_v<move_only_sbo_storage<32, 8>>);
} // namespace poly
#endif
