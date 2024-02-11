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
#ifndef POLY_STOARGE_SBO_STORAGE_HPP
#define POLY_STOARGE_SBO_STORAGE_HPP
#include "poly/config.hpp"
#include "poly/fwd.hpp"

#include <memory>

namespace poly {
namespace detail {

  /// raw storage type for small buffer optimized storage
  /// contains of either a pointer to the heap or the object in the local
  /// buffer
  template<std::size_t Size, std::size_t Align>
  union raw_sbo_storage {
    void* heap;
    alignas(Align) std::byte buffer[Size];
  };

  /// table of function pointers for resource managment used by sbo_storage
  template<bool Copyable>
  struct sbo_resource_table {
    void (*copy)(void* dest,
                 const void* src); ///< copy from one local buffer to another
    void* (*heap_copy)(const void* src); ///< allocates new heap copy
    void (*move)(void* dest,
                 void* src); ///< move from one local buffer to another
    void* (*heap_move)(
        void* src); ///< allocates new heap copy move constructed from src
    void (*destroy)(void* dest);      ///< destroy object in local buffer
    void (*heap_destroy)(void* dest); ///< destroy object on heap
    std::size_t size;
    std::size_t align;
  };

  /// table of function pointers for resource managment used by
  /// sbo_move_only_storage
  template<>
  struct sbo_resource_table<false> {
    void (*move)(void* dest,
                 void* src); ///< move from one local buffer to another
    void* (*heap_move)(
        void* src); ///< allocates new heap copy move constructed from src
    void (*destroy)(void* dest);      ///< destroy object in local buffer
    void (*heap_destroy)(void* dest); ///< destroy object on heap
    std::size_t size;
    std::size_t align;
  };

  /// returns a fully populated sbo_resource_table
  template<bool Copyable, typename T>
  constexpr sbo_resource_table<Copyable> get_sbo_resource_table() noexcept {
    if constexpr (Copyable) {
      return sbo_resource_table<true>{
          // .copy =
          +[](void* dest, const void* src) {
            construct_at(static_cast<T*>(dest), *static_cast<const T*>(src));
          },
          // .heap_copy =
          +[](const void* src) -> void* {
            return allocate<T>(*static_cast<const T*>(src));
          },
          // .move =
          +[](void* dest, void* src) {
            construct_at(static_cast<T*>(dest),
                         std::move(*static_cast<T*>(src)));
          },
          // .heap_move =
          +[](void* src) -> void* {
            return allocate<T>(std::move(*static_cast<T*>(src)));
          },
          // .destroy =
          +[](void* src) { std::destroy_at(static_cast<T*>(src)); },
          // .heap_destroy =
          +[](void* src) { deallocate(static_cast<T*>(src)); },
          // .size =
          sizeof(T),
          // .align =
          alignof(T)};
    } else {
      return sbo_resource_table<false>{
          // .move =
          +[](void* dest, void* src) {
            construct_at(static_cast<T*>(dest),
                         std::move(*static_cast<T*>(src)));
          },
          // .heap_move =
          +[](void* src) -> void* {
            return allocate<T>(std::move(*static_cast<T*>(src)));
          },
          // .destroy =
          +[](void* src) { std::destroy_at(static_cast<T*>(src)); },
          // .heap_destroy =
          +[](void* src) { deallocate(static_cast<T*>(src)); },
          // .size =
          sizeof(T),
          // .align =
          alignof(T)};
    }
  }

  template<bool Copyable, typename T>
  inline constexpr sbo_resource_table<Copyable> sbo_table_for =
      get_sbo_resource_table<Copyable, T>();
  /// storage with small buffer optimization implementation.
  ///
  /// Emplaced objects are allocated within a buffer of Size with alignment
  /// Alignment if the object satisfies these constraints, else the object is
  /// heap allocated.
  /// @tparam Copyable specify if the storage is copyable
  /// @tparam Size  size of the internal buffer in bytes
  /// @tparam Alignment alignment of internal buffer in bytes
  template<bool Copyable, std::size_t Size, std::size_t Alignment>
  class basic_sbo_storage {
  public:
    template<bool C, std::size_t S, std::size_t A>
    friend class basic_sbo_storage;
    template<std::size_t S, std::size_t A>
    friend class sbo_storage;

    constexpr basic_sbo_storage() noexcept {}

    /// copy ctor for copyable sbo storage
    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage(
        const basic_sbo_storage<Copyable, S, A>& other) {
      static_assert(Copyable);
      this->copy(other);
    }

    /// copy ctor for copyable sbo storage
    constexpr basic_sbo_storage(const basic_sbo_storage& other) {
      /// this definition is needed, else the compiler produces a memcpy for the
      /// copy ctor instead of choosing the template version
      static_assert(Copyable);
      this->copy(other);
    }

    /// move ctor
    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage(basic_sbo_storage<Copyable, S, A>&& other) {
      this->move(std::move(other));
    }

    /// move ctor
    constexpr basic_sbo_storage(basic_sbo_storage&& other) {
      this->move(std::move(other));
    }

    /// move assignment
    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage&
    operator=(basic_sbo_storage<Copyable, S, A>&& other) {
      return this->move(std::move(other));
    }

    /// move assignment
    constexpr basic_sbo_storage& operator=(basic_sbo_storage&& other) {
      return this->move(std::move(other));
    }

    /// copy assignment
    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage&
    operator=(const basic_sbo_storage<Copyable, S, A>& other) {
      static_assert(Copyable);
      return this->copy(other);
    }

    /// copy assignment
    constexpr basic_sbo_storage& operator=(const basic_sbo_storage& other) {
      static_assert(Copyable);
      return this->copy(other);
    }

    POLY_CONSTEXPR ~basic_sbo_storage() { reset(); }

    /// create a T with arguments args by either
    /// - in place constructing the T with placment new inside the local
    /// buffer is sizeof(T) <= Size and alignof(T) = Alignment, or
    /// - allocating the T on the heap with operator new if T does not fit
    /// inside the local buffer
    ///
    /// @tparam T type to store
    /// @tparam Args arguments for constructing a T
    /// @returns reference to the stored T
    template<typename T, typename... Args>
    T* emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...> and sizeof(T) <= Size and
        alignof(T) <= Alignment) {
      reset();
      T* ret = nullptr;
      if constexpr (sizeof(T) <= Size and alignof(T) <= Alignment) {
        ret = construct_at(reinterpret_cast<T*>(buffer.buffer),
                           std::forward<Args>(args)...);
        if (!ret)
          return nullptr;
      } else {
        ret = allocate<T>(std::forward<Args>(args)...);
        if (!ret)
          return nullptr;
        buffer.heap = ret;
      }
      vtbl_ = &sbo_table_for<Copyable, T>;
      return ret;
    }

    /// get pointer to contained object, or nullptr if no object was emplaced.
    constexpr void* data() noexcept {
      return this->contains_value() ? this->as<void>() : nullptr;
    }

    /// get pointer to contained object, or nullptr if no object was emplaced.
    constexpr const void* data() const noexcept {
      return this->contains_value() ? this->as<const void>() : nullptr;
    }

  private:
    constexpr void reset() {
      if (not this->contains_value())
        return;

      if (vtbl_->size > Size or vtbl_->align > Alignment)
        vtbl_->heap_destroy(buffer.heap);
      else
        vtbl_->destroy(buffer.buffer);
      vtbl_ = nullptr;
    }

    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage&
    move(basic_sbo_storage<Copyable, S, A>&& other) {
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
          other.vtbl_->move(buffer.buffer, other.buffer.heap);
        } else {
          // move others buffer object into this buffer
          other.vtbl_->move(buffer.buffer,
                            other.buffer.buffer);
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
          buffer.heap = other.vtbl_->heap_move(other.buffer.buffer);
          vtbl_ = other.vtbl_;
        }
      }
      other.reset();
      return *this;
    }

    template<std::size_t S, std::size_t A>
    constexpr basic_sbo_storage&
    copy(const basic_sbo_storage<Copyable, S, A>& other) {
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
          other.vtbl_->copy(buffer.buffer, other.buffer.heap);
        } else {
          // copy others buffer object into this buffer
          other.vtbl_->copy(buffer.buffer,
                            other.buffer.buffer);
        }
      } else {
        // others object does not fit into small buffer
        if (other.is_heap_allocated()) {
          // heap copy
          buffer.heap = other.vtbl_->heap_copy(other.buffer.heap);
        } else {
          // heap copy
          buffer.heap = other.vtbl_->heap_copy(other.buffer.buffer);
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

    template<typename T>
    constexpr T* as() noexcept {
      if (vtbl_->size <= Size and vtbl_->align <= Alignment)
        return static_cast<T*>(
            static_cast<void*>(buffer.buffer));
      return static_cast<T*>(buffer.heap);
    }

    template<typename T>
    constexpr const T* as() const noexcept {
      if (vtbl_->size <= Size and vtbl_->align <= Alignment)
        return static_cast<const T*>(
            static_cast<const void*>(buffer.buffer));
      return static_cast<const T*>(buffer.heap);
    }

    const detail::sbo_resource_table<Copyable>* vtbl_{nullptr};
    detail::raw_sbo_storage<Size, Alignment> buffer;
  };

} // namespace detail

/// Copyable storage with small buffer optimization.
///
/// Emplaced objects are allocated within a buffer of Size with alignment
/// Alignment if the object satisfies these constraints, else it is heap
/// allocated.
/// @tparam Size  size of the internal buffer in bytes
/// @tparam Alignment alignment of internal buffer in bytes
template<std::size_t Size, std::size_t Alignment>
class sbo_storage final
    : public detail::basic_sbo_storage<true, Size, Alignment> {
public:
  template<std::size_t S, std::size_t A>
  friend class sbo_storage;

  using Base = detail::basic_sbo_storage<true, Size, Alignment>;
  using Base::data;
  using Base::emplace;

  /// construct empty storage
  constexpr sbo_storage() noexcept : Base() {}

  /// move ctor
  template<std::size_t S, std::size_t A>
  constexpr sbo_storage(sbo_storage<S, A>&& s) : Base(std::move(s)) {}
  constexpr sbo_storage(sbo_storage&& s) : Base(std::move(s)) {}

  /// copy ctor
  constexpr sbo_storage(const sbo_storage& s) : Base(s) {}
  template<std::size_t S, std::size_t A>
  constexpr sbo_storage(const sbo_storage<S, A>& s) : Base(s) {}

  /// move assignemnt
  template<std::size_t S, std::size_t A>
  constexpr sbo_storage& operator=(sbo_storage<S, A>&& s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// copy assignemnt
  constexpr sbo_storage& operator=(const sbo_storage& s) {
    Base::operator=(s);
    return *this;
  }
  template<std::size_t S, std::size_t A>
  constexpr sbo_storage& operator=(const sbo_storage<S, A>& s) {
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
template<std::size_t Size, std::size_t Alignment>
class move_only_sbo_storage final
    : public detail::basic_sbo_storage<false, Size, Alignment> {
public:
  using Base = detail::basic_sbo_storage<false, Size, Alignment>;
  using Base::data;
  using Base::emplace;
  /// construct empty storage
  constexpr move_only_sbo_storage() noexcept : Base() {}

  /// construct with a T
  // template <typename T, typename = std::enable_if_t<
  //                           not poly::is_storage_v<std::decay_t<T>>>>
  // constexpr move_only_sbo_storage(T &&t) noexcept(
  //     std::is_nothrow_move_constructible_v<std::decay_t<T>>)
  //     : Base(std::forward<T>(t)) {}

  /// move ctor
  template<std::size_t S, std::size_t A>
  constexpr move_only_sbo_storage(move_only_sbo_storage<S, A>&& s)
      : Base(std::move(s)) {}

  /// deleted copy ctor
  move_only_sbo_storage(const move_only_sbo_storage& s) = delete;

  /// move assignment
  // constexpr move_only_sbo_storage &operator=(move_only_sbo_storage &&s) {
  //   Base::operator=(std::move(s));
  //   return *this;
  // }
  template<std::size_t S, std::size_t A>
  constexpr move_only_sbo_storage& operator=(move_only_sbo_storage<S, A>&& s) {
    Base::operator=(std::move(s));
    return *this;
  }
  constexpr move_only_sbo_storage& operator=(move_only_sbo_storage&& s) {
    Base::operator=(std::move(s));
    return *this;
  }

  /// deleted copy assignment
  move_only_sbo_storage& operator=(const move_only_sbo_storage& s) = delete;
};
} // namespace poly
#endif
