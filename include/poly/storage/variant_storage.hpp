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
#ifndef POLY_VARIANT_STORAGE_HPP
#define POLY_VARIANT_STORAGE_HPP
#include "poly/config.hpp"
#include "poly/fwd.hpp"
#include "poly/traits.hpp"
#include "poly/type_list.hpp"

#include <memory>

namespace poly {
namespace detail {

  struct NoValue {};

  template<std::size_t I, typename... Ts>
  union variant_impl {
    constexpr variant_impl() = default;
    variant_impl(const variant_impl&) = delete;
    variant_impl(variant_impl&&) = delete;
    variant_impl& operator=(const variant_impl&) = delete;
    variant_impl& operator=(variant_impl&&) = delete;

    template<typename T, typename... Args>
    constexpr auto* create(Args&&...) {
      static_assert(always_false<T>, "Library bug!");
    }

    void copy(const variant_impl&, std::size_t) {}

    void move(variant_impl&&, std::size_t) {}

    constexpr void destroy(std::size_t) {}
    POLY_CONSTEXPR ~variant_impl() noexcept {}
  };
  template<std::size_t I, typename T1, typename... Ts>
  union variant_impl<I, T1, Ts...> {
    using value_type = T1;
    using types = type_list<T1, Ts...>;
    static constexpr inline bool is_last = sizeof...(Ts) - 1 == I;
    using rest_type =
        std::conditional_t<is_last, NoValue, variant_impl<I + 1, Ts...>>;

    template<typename T>
    static constexpr bool contains = contains_v<types, std::decay_t<T>>;

    rest_type rest_;
    value_type value_;

    // ctor for empty variant
    constexpr variant_impl() noexcept : rest_() {}
    variant_impl(const variant_impl&) = delete;
    variant_impl(variant_impl&&) = delete;
    variant_impl& operator=(const variant_impl&) = delete;
    variant_impl& operator=(variant_impl&&) = delete;

    template<typename T, typename... Args>
    constexpr auto* create(Args&&... args) {
      static_assert(contains<T>,
                    "The type T must be in the list of types the "
                    "variant storage is defined with.");
      if constexpr (std::is_same_v<T, value_type>) {
        return construct_at(std::addressof(value_),
                            std::forward<Args>(args)...);
      } else {
        if constexpr (is_last) {
          static_assert(always_false<T>, "Library bug!");
        } else {
          return rest_.template create<T>(std::forward<Args>(args)...);
        }
      }
    }

    constexpr void copy(const variant_impl& other, std::size_t idx) {
      if (I == idx) {
        construct_at(std::addressof(value_), other.value_);
      } else {
        if constexpr (not is_last)
          rest_.copy(other.rest_, idx);
      }
    }

    constexpr void move(variant_impl&& other, std::size_t idx) {
      if (I == idx) {
        construct_at(std::addressof(value_), std::move(other.value_));
      } else {
        if constexpr (not is_last)
          rest_.move(std::move(other.rest_), idx);
      }
    }

    constexpr void destroy(std::size_t idx) {
      if (I == idx) {
        std::destroy_at(std::addressof(value_));
      } else {
        if constexpr (not is_last)
          rest_.destroy(idx);
      }
    }
    POLY_CONSTEXPR ~variant_impl() noexcept {}
  };

  template<bool Copyable, typename... Ts>
  class variant_storage_impl {
    using index_type = traits::smallest_uint_to_contain<sizeof...(Ts)>;
    variant_impl<0, Ts...> impl_;
    index_type idx;

    using types = type_list<Ts...>;
    template<typename T>
    static constexpr bool contains = contains_v<types, T>;

    template<typename T>
    struct construct_candidates {
      template<typename T_>
      using convertible = std::is_convertible<T, T_>;
      using type = filter_t<types, convertible>;
    };

  public:
    static constexpr bool copyable =
        conjunction_v<transform_t<types, std::is_copy_constructible>>;
    static constexpr bool nothrow_copyable =
        conjunction_v<transform_t<types, std::is_nothrow_copy_constructible>>;
    static constexpr bool nothrow_movable =
        conjunction_v<transform_t<types, std::is_nothrow_move_constructible>>;
    static constexpr bool nothrow_destructible =
        conjunction_v<transform_t<types, std::is_nothrow_destructible>>;

    constexpr variant_storage_impl() noexcept : impl_(), idx(sizeof...(Ts)) {}

    constexpr variant_storage_impl(const variant_storage_impl& other) noexcept(
        nothrow_copyable)
        : variant_storage_impl() {
      impl_.copy(other.impl_, other.idx);
      idx = other.idx;
    }

    constexpr variant_storage_impl(variant_storage_impl&& other) noexcept(
        nothrow_movable)
        : variant_storage_impl() {
      impl_.move(std::move(other.impl_), other.idx);
      other.impl_.destroy(other.idx);
      idx = std::exchange(other.idx, sizeof...(Ts));
    }

    POLY_CONSTEXPR ~variant_storage_impl() {
      if (idx != sizeof...(Ts))
        impl_.destroy(idx);
    }

    constexpr variant_storage_impl&
    operator=(variant_storage_impl&& other) noexcept(nothrow_movable and
                                                     nothrow_destructible) {
      if (this == &other)
        return *this;
      impl_.destroy(idx);
      impl_.move(std::move(other.impl_), other.idx);
      other.impl_.destroy(other.idx);
      idx = std::exchange(other.idx, sizeof...(Ts));
      return *this;
    }

    constexpr variant_storage_impl& operator=(
        const variant_storage_impl& other) noexcept(nothrow_copyable and
                                                    nothrow_destructible) {
      if (this == &other)
        return *this;
      impl_.destroy(idx);
      impl_.copy(other.impl_, other.idx);
      idx = other.idx;
      return *this;
    }

    template<typename T, typename... Args>
    constexpr T*
    emplace(Args&&... args) noexcept(std::is_constructible_v<T, Args&&...>) {
      static_assert(contains_v<types, T>, "T ist not a valid variant option");
      impl_.destroy(idx);
      idx = 0;
      T* t = impl_.template create<T>(std::forward<Args>(args)...);
      idx = index_of_v<types, T>;
      return t;
    }

    constexpr void* data() noexcept {
      return idx != sizeof...(Ts) ? static_cast<void*>(&impl_) : nullptr;
    }

    constexpr const void* data() const noexcept {
      return idx != sizeof...(Ts) ? static_cast<const void*>(&impl_) : nullptr;
    }
  };
  template<typename... Ts>
  class variant_storage_impl<false, Ts...> {
    using index_type = traits::smallest_uint_to_contain<sizeof...(Ts)>;
    variant_impl<0, Ts...> impl_;
    index_type idx;

    using types = type_list<Ts...>;
    template<typename T>
    static constexpr bool contains = contains_v<types, T>;

  public:
    static constexpr bool copyable =
        conjunction_v<transform_t<types, std::is_copy_constructible>>;
    static constexpr bool nothrow_copyable =
        conjunction_v<transform_t<types, std::is_nothrow_copy_constructible>>;
    static constexpr bool nothrow_movable =
        conjunction_v<transform_t<types, std::is_nothrow_move_constructible>>;
    static constexpr bool nothrow_destructible =
        conjunction_v<transform_t<types, std::is_nothrow_destructible>>;

    constexpr variant_storage_impl() noexcept : impl_(), idx(sizeof...(Ts)) {}

    constexpr variant_storage_impl(const variant_storage_impl& other) = delete;
    constexpr variant_storage_impl(variant_storage_impl&& other) noexcept(
        nothrow_movable)
        : variant_storage_impl() {
      impl_.move(std::move(other.impl_), other.idx);
      other.impl_.destroy(other.idx);
      idx = std::exchange(other.idx, sizeof...(Ts));
    }

    POLY_CONSTEXPR ~variant_storage_impl() {
      if (idx != sizeof...(Ts))
        impl_.destroy(idx);
    }

    constexpr variant_storage_impl&
    operator=(variant_storage_impl&& other) noexcept(nothrow_movable and
                                                     nothrow_destructible) {
      if (this == &other)
        return *this;
      impl_.destroy(idx);
      impl_.move(std::move(other.impl_), other.idx);
      other.impl_.destroy(other.idx);
      idx = std::exchange(other.idx, sizeof...(Ts));
      return *this;
    }

    constexpr variant_storage_impl&
    operator=(const variant_storage_impl& other) = delete;

    template<typename T, typename... Args>
    constexpr T*
    emplace(Args&&... args) noexcept(std::is_constructible_v<T, Args&&...>) {
      static_assert(contains_v<types, T>, "T ist not a valid variant option");
      impl_.destroy(idx);
      idx = 0;
      T* t = impl_.template create<T>(std::forward<Args>(args)...);
      idx = index_of_v<types, T>;
      return t;
    }

    constexpr void* data() noexcept {
      return idx != sizeof...(Ts) ? static_cast<void*>(&impl_) : nullptr;
    }

    constexpr const void* data() const noexcept {
      return idx != sizeof...(Ts) ? static_cast<const void*>(&impl_) : nullptr;
    }
  };
} // namespace detail
/// The variant storage can store an object of type T, if T is in the pack Ts.
/// Does not depend on std::variant.
///
/// The advantage of using this kind of storage, compared to local_storage, is
/// that there is no need to generate a static resource table.
///
/// Like std::variant, it essentially is a tagged union. The key difference to
/// std::variant is that the data() member function evaluates only one
/// conditional (to check if something is contained) and returning the address
/// of the internal union as a void* if there is something emplaced, instead
/// of having to rely on std::visit.
template<typename... Ts>
class variant_storage : public detail::variant_storage_impl<
                            (std::is_copy_constructible_v<Ts> && ...), Ts...> {
public:
  using Base =
      detail::variant_storage_impl<(std::is_copy_constructible_v<Ts> && ...),
                                   Ts...>;
  using Base::Base;
  using Base::data;

  constexpr variant_storage() noexcept = default;
  variant_storage(const variant_storage& other) noexcept(
      (std::is_nothrow_copy_constructible_v<Ts> && ...))
      : Base(other) {}

  variant_storage(variant_storage&& other) noexcept(
      (std::is_nothrow_move_constructible_v<Ts> && ...))
      : Base(std::move(other)) {}

  variant_storage& operator=(const variant_storage& other) noexcept(
      (std::is_nothrow_copy_constructible_v<Ts> && ...)) {
    Base::operator=(other);
    return *this;
  }

  variant_storage& operator=(variant_storage&& other) noexcept(
      (std::is_nothrow_move_constructible_v<Ts> && ...)) {
    Base::operator=(std::move(other));
    return *this;
  }
};
static_assert(is_storage_v<variant_storage<int, double, float>>);

} // namespace poly
#endif
