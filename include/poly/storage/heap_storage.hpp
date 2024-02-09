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
#ifndef POLY_HEAP_STORAGE_HPP
#define POLY_HEAP_STORAGE_HPP
#include "poly/alloc.hpp"
#include "poly/config.hpp"
#include "poly/fwd.hpp"
#include "poly/traits.hpp"
#include <cassert>

namespace poly {
namespace detail {

  /// heap_storage contains a pointer to a basic_heap_block.
  /// @{
  template<bool Copyable>
  struct basic_heap_block {
    void* obj{nullptr};
    basic_heap_block* (*copy_)(const basic_heap_block*){nullptr};
    void (*destroy_)(basic_heap_block*){nullptr};
    basic_heap_block* copy() { return this->copy_(this); }
    void destroy() { this->destroy_(this); }
  };
  template<>
  struct basic_heap_block<false> {
    void* obj{nullptr};
    void (*destroy_)(basic_heap_block* obj){nullptr};
    void destroy() { this->destroy_(this); }
  };

  /// actual type allocated when using heap_storage.
  /// templated only on size and alignment of a type, as the destructor is
  /// stored in the basic_heap_block.
  template<bool Copyable, std::size_t Size, std::size_t Align>
  struct heap_block : basic_heap_block<Copyable> {
    constexpr heap_block() = default;

    template<typename T, typename... Args, bool C = Copyable,
             typename = std::enable_if_t<C>>
    heap_block(traits::Id<T>, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args&&...>)
        : basic_heap_block<Copyable>{
              nullptr,
              +[](const basic_heap_block<Copyable>* o)
                  -> basic_heap_block<Copyable>* {
                assert(o);
                const auto* block = static_cast<const heap_block*>(o);
                // allocating memory
                void* mem = mem_alloc(sizeof(heap_block), alignof(heap_block));
                // default construction of heap_blcok into allcoateds memory
                auto* ret = construct_at(reinterpret_cast<heap_block*>(mem));
                // reassign data members
                ret->copy_ = block->copy_;
                ret->destroy_ = block->destroy_;
                // copying the T in buffer
                ret->obj =
                    construct_at(reinterpret_cast<T*>(ret->buffer),
                                 *reinterpret_cast<const T*>(block->buffer));
                return ret;
              },
              +[](basic_heap_block<Copyable>* o) {
                std::destroy_at(static_cast<T*>(o->obj));
                mem_free(o);
              }} {
      this->obj = construct_at(reinterpret_cast<T*>(buffer),
                               std::forward<Args>(args)...);
    }

    template<typename T, typename... Args, bool C = Copyable, bool NC = !C,
             typename = std::enable_if_t<NC>>
    heap_block(traits::Id<T>, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args&&...>)
        : basic_heap_block<Copyable>{nullptr,
                                     +[](basic_heap_block<Copyable>* o) {
                                       std::destroy_at(static_cast<T*>(o->obj));
                                       mem_free(o);
                                     }} {
      this->obj = construct_at(reinterpret_cast<T*>(buffer),
                               std::forward<Args>(args)...);
    }

    alignas(Align) std::byte buffer[Size]{};
  };

  /// allocates a heap_block for a T and constructs it with args.
  template<bool Copyable, typename T, typename... Args>
  basic_heap_block<Copyable>* allocate_block(
      traits::Id<T> id,
      Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
    using block = heap_block<Copyable, sizeof(T), alignof(T)>;
    void* mem = mem_alloc(sizeof(block), alignof(block));
    if (mem == nullptr)
      return nullptr;
    return ::new (mem) block(id, std::forward<Args>(args)...);
  }

  template<bool Copyable>
  class basic_heap_storage {
  public:
    constexpr basic_heap_storage() noexcept = default;

    basic_heap_storage(const basic_heap_storage& other)
        : block_(other.block_ ? other.block_->copy() : nullptr) {}

    basic_heap_storage(basic_heap_storage&& other) noexcept
        : block_(std::exchange(other.block_, nullptr)) {}

    ~basic_heap_storage() { reset(); }

    basic_heap_storage& operator=(const basic_heap_storage& other) {
      if (this == &other)
        return *this;
      reset();
      block_ = other.block_ ? other.block_->copy() : nullptr;
      return *this;
    }

    basic_heap_storage& operator=(basic_heap_storage&& other) noexcept {
      if (this == &other)
        return *this;
      std::swap(block_, other.block_);
      other.reset();
      return *this;
    }

    template<typename T, typename... Args>
    T* emplace(Args&&... args) {
      reset();
      block_ = detail::allocate_block<Copyable>(traits::Id<T>{},
                                                std::forward<Args>(args)...);
      return block_ ? static_cast<T*>(block_->obj) : nullptr;
    }

    void* data() noexcept { return block_ ? block_->obj : nullptr; }
    const void* data() const noexcept { return block_ ? block_->obj : nullptr; }

  private:
    void reset() noexcept {
      if (block_ == nullptr)
        return;
      block_->destroy(); // also deallocates block itself
      block_ = nullptr;
    }
    using block = detail::basic_heap_block<Copyable>;
    block* block_{nullptr};
  };

} // namespace detail
using heap_storage = detail::basic_heap_storage<true>;
using move_only_heap_storage = detail::basic_heap_storage<false>;
} // namespace poly
#endif
