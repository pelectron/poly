/**
 * Copyright 2024 Pelé Constam
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef POLY_STORAGE_LOCAL_STORAGE_HPP
#define POLY_STORAGE_LOCAL_STORAGE_HPP
#include "poly/alloc.hpp"
#include "poly/config.hpp"
#include "poly/fwd.hpp"

#include <utility>

namespace poly {
namespace detail {
  /// table of function pointers for resource management used by local_storage
  template<bool Copyable>
  struct resource_table {
    void (*copy)(void* dest, const void* src);
    void (*move)(void* dest, void* src);
    void (*destroy)(void* dest);
  };

  /// table of function pointers for resource management used by
  /// local_move_only_storage
  template<>
  struct resource_table<false> {
    void (*move)(void* dest, void* src);
    void (*destroy)(void* dest);
  };

  /// returns a fully populated resource_table
  template<bool Copyable, typename T>
  constexpr resource_table<Copyable> get_local_resource_table() noexcept {
    if constexpr (Copyable) {
      return resource_table<true>{
          //.copy =
          +[](void* dest, const void* src) {
            poly::detail::construct_at(static_cast<T*>(dest),
                                       *static_cast<const T*>(src));
          },
          //.move =
          +[](void* dest, void* src) {
            poly::detail::construct_at(static_cast<T*>(dest),
                                       std::move(*static_cast<T*>(src)));
          },
          //.destroy =
          +[](void* src) { std::destroy_at(static_cast<T*>(src)); }};
    } else {
      return resource_table<false>{
          //.move =
          +[](void* dest, void* src) {
            poly::detail::construct_at(static_cast<T*>(dest),
                                       std::move(*static_cast<T*>(src)));
          },
          //.destroy =
          +[](void* src) { std::destroy_at(static_cast<T*>(src)); }};
    }
  }

  template<bool Copyable, typename T>
  inline constexpr resource_table<Copyable> resource_table_for =
      get_local_resource_table<Copyable, T>();

  /// local storage implementation.
  ///
  /// Holds object emplaced in a buffer of Size bytes with an alignment of
  /// Alignment. Objects greater than Size or stricter alignment than Alignment
  /// cannot be emplaced.
  /// @tparam Copyable specify if the storage is copyable
  /// @tparam Size  size of the internal buffer in bytes
  /// @tparam Alignment alignment of internal buffer in bytes
  template<bool Copyable, std::size_t Size, std::size_t Alignment>
  class basic_local_storage {
  public:
    template<bool C, std::size_t S, std::size_t A>
    friend class basic_local_storage;

    constexpr basic_local_storage() noexcept = default;

    /// copy ctor
    template<std::size_t S, std::size_t A, bool C = Copyable>
    constexpr basic_local_storage(const basic_local_storage<C, S, A>& other) {
      static_assert(Copyable);
      this->copy(other);
    }
    /// copy ctor
    constexpr basic_local_storage(const basic_local_storage& other) {
      /// this definition is needed, else the compiler produces a memcpy for the
      /// copy ctor instead of choosing the template version
      static_assert(Copyable);
      this->copy(other);
    }

    /// move ctor
    template<std::size_t S, std::size_t A>
    constexpr basic_local_storage(basic_local_storage<Copyable, S, A>&& other) {
      this->move(std::move(other));
    }

    /// copy assignment
    template<std::size_t S, std::size_t A>
    constexpr basic_local_storage&
    operator=(const basic_local_storage<Copyable, S, A>& other) {
      static_assert(Copyable);
      return this->copy(other);
    }

    /// copy assignment
    constexpr basic_local_storage& operator=(const basic_local_storage& other) {
      static_assert(Copyable);
      return this->copy(other);
    }

    /// move assignment
    template<std::size_t S, std::size_t A>
    constexpr basic_local_storage&
    operator=(basic_local_storage<Copyable, S, A>&& other) {
      static_assert(
          S <= Size,
          "The local_storage to copy from is too big to fit into this");
      static_assert(A <= Alignment,
                    "The alignemnt of the local_storage to copy "
                    "from is too big to fit into this");
      return this->move(std::move(other));
    }

    POLY_CONSTEXPR ~basic_local_storage() { reset(); }

    /// create a T with arguments args by in place constructing the T with
    /// placment new inside the local buffer if sizeof(T) <= Size and alignof(T)
    /// = Alignment.
    ///
    /// @tparam T type to store
    /// @tparam Args arguments for constructing a T
    /// @returns reference to the stored T
    template<typename T, typename... Args>
    T* emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
      static_assert(sizeof(T) <= Size, "T is to large to fit into this");
      static_assert(alignof(T) <= Alignment,
                    "The alignment of T is to large to fit into this");
      reset();
      T* ret = poly::detail::construct_at(static_cast<T*>(buffer_),
                                          std::forward<Args>(args)...);
      if (!ret)
        return nullptr;
      vtbl_ = &resource_table_for<Copyable, T>;
      return ret;
    }

    /// get pointer to contained object, or nullptr if no object was emplaced.
    constexpr void* data() noexcept {
      return vtbl_ ? this->as<void>() : nullptr;
    }

    /// get pointer to contained object, or nullptr if no object was emplaced.
    constexpr const void* data() const noexcept {
      return vtbl_ ? this->as<const void>() : nullptr;
    }

  private:
    /// destroy the contained object
    constexpr void reset() {
      if (vtbl_) {
        vtbl_->destroy(buffer_);
        vtbl_ = nullptr;
      }
    }

    template<typename T>
    constexpr T* as() noexcept {
      return static_cast<T*>(static_cast<void*>(buffer_));
    }

    template<typename T>
    constexpr const T* as() const noexcept {
      return static_cast<const T*>(static_cast<const void*>(buffer_));
    }

    /// move implementation
    template<std::size_t S, std::size_t A>
    constexpr basic_local_storage&
    move(basic_local_storage<Copyable, S, A>&& other) {
      static_assert(
          S <= Size,
          "The local_storage to copy from is too big to fit into this");
      static_assert(A <= Alignment,
                    "The alignemnt of the local_storage to copy "
                    "from is too big to fit into this");
      if constexpr (S == Size and A == Alignment) {
        // only check for this if other is of same type
        if (this == &other)
          return *this;
      }
      reset();
      if (other.vtbl_ == nullptr)
        return *this;
      other.vtbl_->move(buffer_, other.buffer_);
      vtbl_ = other.vtbl_;
      other.reset();
      return *this;
    }

    /// copy implementation
    template<std::size_t S, std::size_t A>
    constexpr basic_local_storage&
    copy(const basic_local_storage<Copyable, S, A>& other) {
      static_assert(
          S <= Size,
          "The local_storage to copy from is too big to fit into this");
      static_assert(A <= Alignment,
                    "The alignemnt of the local_storage to copy "
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
      other.vtbl_->copy(buffer_, other.buffer_);
      vtbl_ = other.vtbl_;
      return *this;
    }

    const resource_table<Copyable>* vtbl_{nullptr};

    alignas(Alignment) std::byte buffer_[Size]{};
  };
} // namespace detail

/// owing, copyable storage without dynamic allocation.
///
/// Holds object emplaced in a buffer of Size bytes with an alignment of
/// Alignment. Objects greater than Size or stricter alignment than Alignment
/// cannot be emplaced.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template<std::size_t Size, std::size_t Alignment>
class local_storage final
    : public detail::basic_local_storage<true, Size, Alignment> {
public:
  using Base = detail::basic_local_storage<true, Size, Alignment>;
  using Base::data;
  using Base::emplace;

  /// construct empty storage
  constexpr local_storage() noexcept : Base() {}

  /// move ctor
  constexpr local_storage(local_storage&& s) : Base(std::move(s)) {}
  template<std::size_t S, std::size_t A>
  constexpr local_storage(local_storage<S, A>&& s) : Base(std::move(s)) {}

  /// copy ctor
  template<std::size_t S, std::size_t A>
  constexpr local_storage(const local_storage<S, A>& s) : Base(s) {}
  constexpr local_storage(const local_storage& s) : Base(s) {}

  /// move assignment
  template<std::size_t S, std::size_t A>
  constexpr local_storage& operator=(local_storage<S, A>&& s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// copy assignment
  template<std::size_t S, std::size_t A>
  constexpr local_storage& operator=(const local_storage<A, S>& s) {
    Base::operator=(s);
    return *this;
  }
  /// copy assignment
  constexpr local_storage& operator=(const local_storage& s) {
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
template<std::size_t Size, std::size_t Alignment>
class move_only_local_storage final
    : public detail::basic_local_storage<false, Size, Alignment> {
public:
  using Base = detail::basic_local_storage<false, Size, Alignment>;
  using Base::data;
  using Base::emplace;

  /// construct empty storage
  constexpr move_only_local_storage() noexcept : Base() {}

  /// move ctor
  template<std::size_t S, std::size_t A>
  constexpr move_only_local_storage(move_only_local_storage<S, A>&& s)
      : Base(std::move(s)) {}

  /// deleted copy ctor
  move_only_local_storage(const move_only_local_storage& s) = delete;

  /// move assignment
  template<std::size_t S, std::size_t A>
  constexpr move_only_local_storage&
  operator=(move_only_local_storage<S, A>&& s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// deleted copy assignment
  move_only_local_storage& operator=(const move_only_local_storage& s) = delete;
};
} // namespace poly
#endif
