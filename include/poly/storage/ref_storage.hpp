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
#ifndef POLY_REF_STORAGE_HPP
#define POLY_REF_STORAGE_HPP
#include "poly/config.hpp"
#include "poly/fwd.hpp"
#include "poly/traits.hpp"

namespace poly {
/// non owing storage. Only contains pointer to object emplaced. Can be
/// constructed from any non const lvalue reference to any T or Storage type.
class POLY_API ref_storage final {
public:
  template<typename T>
  constexpr ref_storage(T& t) noexcept
      : ref_storage(t, poly::traits::is_storage<T>{}) {}

  constexpr ref_storage() noexcept = default;

  constexpr ref_storage(const ref_storage&) noexcept = default;

  constexpr ref_storage(ref_storage&&) noexcept = default;

  constexpr ref_storage& operator=(const ref_storage&) noexcept = default;

  constexpr ref_storage& operator=(ref_storage&&) noexcept = default;

  template<typename T>
  constexpr T* emplace(T& t) noexcept {
    ref_ = &t;
    return &t;
  }

  constexpr void* data() noexcept { return ref_; }

  constexpr const void* data() const noexcept { return ref_; }

private:

  constexpr void reset() noexcept { ref_ = nullptr; }

  template<typename T>
  constexpr ref_storage(T& t, std::false_type /*is_storage*/)
      : ref_(&t) {}

  template<POLY_STORAGE Storage>
  constexpr ref_storage(Storage& s, std::true_type /*is_storage*/)
      : ref_(s.data()) {}

  void* ref_{nullptr};
};

static_assert(poly::traits::is_storage_v<ref_storage>);
} // namespace poly
#endif
