/**
 *  Copyright 2024 Pelé Constam
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef POLY_OBJECT_HPP
#define POLY_OBJECT_HPP
#include "poly/object_method.hpp"
#include "poly/object_property.hpp"
#include "poly/storage.hpp"

namespace poly {
namespace detail {
template <typename Storage, typename T, typename... Args>
inline constexpr bool nothrow_emplaceable_v = noexcept(
    std::declval<Storage>().template emplace<T>(std::declval<Args>()...));

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
struct object_table;
template <template <typename...> typename L, POLY_PROP_SPEC... PropertySpecs,
          POLY_METHOD_SPEC... MethodSpecs>
struct object_table<L<PropertySpecs...>, L<MethodSpecs...>>
    : VTable<MethodSpecs...>, PTable<PropertySpecs...> {
  using vtable_type = VTable<MethodSpecs...>;
  using ptable_type = PTable<PropertySpecs...>;

  template <typename T>
  constexpr object_table(poly::traits::Id<T> id) noexcept
      : VTable<MethodSpecs...>(id), PTable<PropertySpecs...>(id) {}
  constexpr object_table() = default;

  template <typename T>
  static method_offset_type method_offset(traits::Id<T>) noexcept {
    // no offset for first base -> return VTable::method_offset() directly
    return VTable<MethodSpecs...>::method_offset(traits::Id<T>{});
  }
  template <typename T>
  static property_offset_type property_offset(traits::Id<T>) noexcept {
    constexpr object_table<L<PropertySpecs...>, L<MethodSpecs...>> t;
    const std::byte *this_ =
        static_cast<const std::byte *>(static_cast<const void *>(&t));
    const std::byte *ptable =
        static_cast<const std::byte *>(static_cast<const void *>(
            static_cast<const PTable<PropertySpecs...> *>(&t)));
    const size_t table_offset = ptable - this_;
    return table_offset +
           PTable<PropertySpecs...>::property_offset(traits::Id<T>{});
  }
};

template <typename T, POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
inline constexpr object_table object_table_for =
    object_table<PropertySpecs, MethodSpecs>(poly::traits::Id<T>{});

template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs, POLY_TYPE_LIST OverLoads>
struct basic_interface_impl;

template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs, POLY_TYPE_LIST Overloads>
struct POLY_EMPTY_BASE basic_object_impl;

template <POLY_STORAGE StorageType, template <typename...> typename L,
          POLY_PROP_SPEC... PropertySpecs, POLY_TYPE_LIST MethodSpecs,
          typename... OverLoads>
struct POLY_EMPTY_BASE basic_object_impl<StorageType, L<PropertySpecs...>,
                                         MethodSpecs, L<OverLoads...>>
    : public detail::MethodInjector<
          OverLoads, basic_object_impl<StorageType, L<PropertySpecs...>,
                                       MethodSpecs, L<OverLoads...>>>...,
      detail::property_injector_for_t<
          PropertySpecs, basic_object_impl<StorageType, L<PropertySpecs...>,
                                           MethodSpecs, L<OverLoads...>>>... {

  template <POLY_STORAGE, POLY_TYPE_LIST, POLY_TYPE_LIST, POLY_TYPE_LIST>
  friend struct poly::detail::basic_interface_impl;

public:
  using method_specs = MethodSpecs;
  using property_specs = L<PropertySpecs...>;

  using vtable_type =
      typename detail::object_table<property_specs, method_specs>::vtable_type;
  using ptable_type =
      typename detail::object_table<property_specs, method_specs>::ptable_type;

  template <typename MethodName, typename... Args>
  static constexpr bool nothrow_callable =
      noexcept((*std::declval<const vtable_type *>())(
          MethodName{}, std::declval<void *>(), std::declval<Args>()...));

  template <typename Name, POLY_TYPE_LIST PSpecs> struct spec_by_name {
    template <typename T>
    using predicate = std::is_same<property_name_t<T>, Name>;
    using list = filter_t<PSpecs, predicate>;
    static_assert(detail::list_size<list>::value == 1,
                  "No property with such name exists");
    using type = at_t<list, 0>;
  };
  template <typename Name>
  using spec_for = typename spec_by_name<Name, L<PropertySpecs...>>::type;
  template <typename Name> using value_type_for = value_type_t<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

  static_assert(
      poly::is_type_list_v<MethodSpecs>,
      "The MethodSpecs must be provided as a poly::type_list of MethodSpec.");
  static_assert(poly::is_storage_v<StorageType>,
                "StorageType must satisfy the Storage concept.");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<method_specs, traits::is_method_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<property_specs, traits::is_property_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");

  constexpr basic_object_impl() noexcept = default;

  /// copy ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  constexpr basic_object_impl(
      const basic_object_impl<OtherStorage, property_specs, method_specs,
                              L<OverLoads...>> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : storage_(other.storage_), vtbl_(other.vtbl_) {}
  /// @}

  /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
  /// storage type)
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &>>>
  constexpr basic_object_impl(
      basic_object_impl<OtherStorage, property_specs, method_specs,
                        L<OverLoads...>> &other) noexcept
      : storage_(other.storage_), vtbl_(other.vtbl_) {}

  /// move ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &&>>>
  constexpr basic_object_impl(
      basic_object_impl<OtherStorage, property_specs, method_specs,
                        L<OverLoads...>>
          &&other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage &&>)
      : storage_(std::move(other.storage_)),
        vtbl_(std::exchange(other.vtbl_, nullptr)) {}
  constexpr basic_object_impl(basic_object_impl &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, StorageType &&>)
      : storage_(std::move(other.storage_)),
        vtbl_(std::exchange(other.vtbl_, nullptr)) {}
  /// @}

  /// construct from a T
  /// @{
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_object_impl(T &&t) noexcept(
      detail::nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                                    decltype(t)>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    vtbl_ = &detail::object_table_for<std::decay_t<T>, property_specs,
                                      method_specs>;
  }

  /// in place constructing a T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_object_impl, std::decay_t<T>>>>
  constexpr basic_object_impl(traits::Id<T>, Args &&...args) noexcept(
      detail::nothrow_emplaceable_v<StorageType, T, decltype(args)...>) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
    vtbl_ = &detail::object_table_for<std::decay_t<T>, property_specs,
                                      method_specs>;
  }
  /// @}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, const OtherStorage &>>>
  constexpr basic_object_impl &operator=(
      const basic_object_impl<OtherStorage, property_specs, method_specs,
                              L<OverLoads...>>
          &other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                        const OtherStorage &>) {
    storage_ = other.storage_;
    vtbl_ = other.vtbl_;
    return *this;
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, OtherStorage &&>>>
  constexpr basic_object_impl &
  operator=(basic_object_impl<OtherStorage, property_specs, method_specs,
                              L<OverLoads...>> &&
                other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                             OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    vtbl_ = std::exchange(other.vtbl_, nullptr);
    return *this;
  }

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_object_impl, std::decay_t<T>>>>
  constexpr basic_object_impl &
  operator=(T &&t) noexcept(detail::nothrow_emplaceable_v<
                            StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    vtbl_ = &detail::object_table_for<std::decay_t<T>, property_specs,
                                      method_specs>;
    return *this;
  }

  template <typename MethodName, typename... Args>
  constexpr decltype(auto)
  call(Args &&...args) noexcept(nothrow_callable<MethodName, Args...>) {
    assert(vtable());
    return (*vtable())(MethodName{}, storage_.data(),
                       std::forward<Args>(args)...);
  }

  template <typename MethodName, typename... Args>
  constexpr decltype(auto) call(Args &&...args) const
      noexcept(nothrow_callable<MethodName, Args...>) {
    assert(vtable());
    return (*vtable())(MethodName{}, storage_.data(),
                       std::forward<Args>(args)...);
  }

  template <typename Name, typename = std::enable_if_t<not is_const<Name>>>
  constexpr bool
  set(const value_type_for<Name> &value) noexcept(is_nothrow<Name>) {
    return ptable()->set(Name{}, storage_.data(), value);
  }
  template <typename Name>
  constexpr value_type_for<Name> get() const noexcept(is_nothrow<Name>) {
    return ptable()->get(Name{}, storage_.data());
  }

private:
  constexpr const vtable_type *vtable() const noexcept { return vtbl_; }
  constexpr const ptable_type *ptable() const noexcept { return vtbl_; }

  using object_table = detail::object_table<property_specs, method_specs>;
  const object_table *vtbl_;
  StorageType storage_{};
};
} // namespace detail
/// @addtogroup object Objects
/// Objects (note the capital O) are the core class templates this library
/// provides.
///
/// Objects provide methods and properties described in their lists of @ref
/// MethodSpec "MethodSpecs" and @ref PropertySpec "PropertySpecs".
///
/// Properties model member variables and are defined by @ref PropertySpec
/// "property specs". These specs describe the access (const/mutable), name, and
/// value type of the property.
///
/// A property of an Object with name **n** can be accessed by the
/// Objects set<**n**> and get<**n**> member functions. If name
/// injection is enabled, and the name **n** is created with the
/// @ref POLY_PROPERTY macro, the property can be accessed as if
/// it were a normal c++ member variable of that Object, i.e. ``auto v = obj.n``
/// and ``obj.n = v`` instead of ``auto v = obj.get<n>()`` and
/// ``obj.set<n>(v)``.
///
/// Methods model member functions. They are invoked through an
/// Objects/Interfaces call member function.
///
/// @{

/// This class implements the polymorphic behaviour.
///
/// It holds objects in a storage of type StorageType, and provides the
/// properties and methods given by the list of PropertySpecs and MethodSpecs.
///
/// Objects can only be created and assigned to from types implementing the
/// properties and methods specified by the PropertySpecs and MethodSpecs, and
/// from other Objects with IDENTICAL PropertySpecs and MethodSpecs. Objects
/// cannot be created from Interfaces.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// poly::Storage concept.
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
/// @{
template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
using basic_object = detail::basic_object_impl<
    StorageType, PropertySpecs, MethodSpecs,
    typename detail::collapse_overloads<MethodSpecs>::type>;

/// A Reference is a non owning @ref basic_object "Object" and cheap to copy.
///
/// References can only be created and assigned to from types implementing the
/// properties and methods specified by the PropertySpecs and MethodSpecs, and
/// from other @ref basic_object "Objects" and References with IDENTICAL
/// PropertySpecs and MethodSpecs. References cannot be created from Interfaces.
///
/// @warning  References do not own the bound object, just like normal C++
/// references. Bound objects must be alive for as long as methods and
/// properties of the Reference are accessed.
///
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
using Reference = basic_object<ref_storage, PropertySpecs, MethodSpecs>;

/// @}

} // namespace poly
#endif
