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
#include "poly/method_table.hpp"
#include "poly/property_table.hpp"
#include "poly/storage.hpp"
#include <type_traits>

namespace poly {
namespace detail {
  template<typename Storage, typename T, typename... Args>
  inline constexpr bool nothrow_emplaceable_v = noexcept(
      std::declval<Storage>().template emplace<T>(std::declval<Args>()...));

  template<POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
  struct struct_table;
  template<template<typename...> typename L, POLY_PROP_SPEC... PropertySpecs,
           POLY_METHOD_SPEC... MethodSpecs>
  struct struct_table<L<PropertySpecs...>, L<MethodSpecs...>>
      : method_table<MethodSpecs...>, property_table<PropertySpecs...> {
    using vtable_type = method_table<MethodSpecs...>;
    using ptable_type = property_table<PropertySpecs...>;

    template<typename T>
    constexpr struct_table(poly::traits::Id<T> id) noexcept
        : method_table<MethodSpecs...>(id),
          property_table<PropertySpecs...>(id) {}
    constexpr struct_table() = default;

    template<typename T>
    static method_offset_type method_offset(traits::Id<T>) noexcept {
      // no offset for first base -> return VTable::method_offset() directly
      return method_table<MethodSpecs...>::method_offset(traits::Id<T>{});
    }
    template<typename T>
    static property_offset_type property_offset(traits::Id<T>) noexcept {
      constexpr struct_table<L<PropertySpecs...>, L<MethodSpecs...>> t;
      const std::byte* this_ =
          static_cast<const std::byte*>(static_cast<const void*>(&t));
      const std::byte* ptable =
          static_cast<const std::byte*>(static_cast<const void*>(
              static_cast<const property_table<PropertySpecs...>*>(&t)));
      const size_t table_offset = ptable - this_;
      return table_offset +
             property_table<PropertySpecs...>::property_offset(traits::Id<T>{});
    }
  };

  template<typename T, POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
  inline constexpr struct_table struct_table_for =
      struct_table<PropertySpecs, MethodSpecs>(poly::traits::Id<T>{});

  template<POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
           POLY_TYPE_LIST MethodSpecs, POLY_TYPE_LIST OverLoads>
  struct POLY_EMPTY_BASE interface_impl;

  template<POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
           POLY_TYPE_LIST MethodSpecs, POLY_TYPE_LIST Overloads>
  struct POLY_EMPTY_BASE struct_impl;

  template<POLY_STORAGE StorageType, template<typename...> typename L,
           POLY_PROP_SPEC... PropertySpecs, POLY_TYPE_LIST MethodSpecs,
           typename... OverLoads>
  struct POLY_EMPTY_BASE struct_impl<StorageType, L<PropertySpecs...>,
                                     MethodSpecs, L<OverLoads...>>
      : public detail::method_injector_for_t<
            struct_impl<StorageType, L<PropertySpecs...>, MethodSpecs,
                        L<OverLoads...>>,
            OverLoads>...,
        detail::property_injector_for_t<
            struct_impl<StorageType, L<PropertySpecs...>, MethodSpecs,
                        L<OverLoads...>>,
            PropertySpecs>... {

    template<POLY_STORAGE, POLY_TYPE_LIST, POLY_TYPE_LIST, POLY_TYPE_LIST>
    friend struct poly::detail::interface_impl;

  public:
    using method_specs = MethodSpecs;
    using property_specs = L<PropertySpecs...>;

    using vtable_type =
        typename detail::struct_table<property_specs,
                                      method_specs>::vtable_type;
    using ptable_type =
        typename detail::struct_table<property_specs,
                                      method_specs>::ptable_type;

    template<typename MethodName, typename... Args>
    static constexpr bool nothrow_callable =
        noexcept((*std::declval<const vtable_type*>())(
            MethodName{}, std::declval<void*>(), std::declval<Args>()...));

    template<typename Name, POLY_TYPE_LIST PSpecs>
    struct spec_by_name {
      template<typename T>
      using predicate = std::is_same<property_name_t<T>, Name>;
      using list = filter_t<PSpecs, predicate>;
      static_assert(detail::list_size<list>::value == 1,
                    "No property with such name exists");
      using type = at_t<list, 0>;
    };
    template<typename Name>
    using spec_for = typename spec_by_name<Name, L<PropertySpecs...>>::type;
    template<typename Name>
    using value_type_for = value_type_t<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

    static_assert(poly::is_type_list_v<MethodSpecs>,
                  "The MethodSpecs must be provided as a poly::type_list of "
                  "MethodSpec.");
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

    constexpr struct_impl() noexcept = default;

    /// copy ctor
    /// @{
    constexpr struct_impl(const struct_impl& other) noexcept(
        std::is_nothrow_copy_constructible_v<StorageType>)
        : storage_(other.storage_), vtbl_(other.vtbl_) {}

    template<typename OtherStorage,
             typename = std::enable_if_t<
                 std::is_constructible_v<StorageType, const OtherStorage&>>>
    constexpr struct_impl(
        const struct_impl<OtherStorage, property_specs, method_specs,
                          L<OverLoads...>>&
            other) noexcept(std::
                                is_nothrow_constructible_v<StorageType,
                                                           const OtherStorage&>)
        : storage_(other.storage_), vtbl_(other.vtbl_) {}
    /// @}

    /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
    /// storage type)
    template<typename OtherStorage,
             typename = std::enable_if_t<
                 std::is_constructible_v<StorageType, OtherStorage&>>>
    constexpr struct_impl(
        struct_impl<OtherStorage, property_specs, method_specs,
                    L<OverLoads...>>& other) noexcept
        : storage_(other.storage_), vtbl_(other.vtbl_) {}

    /// move ctor
    /// @{
    template<typename OtherStorage,
             typename = std::enable_if_t<
                 std::is_constructible_v<StorageType, OtherStorage&&>>>
    constexpr struct_impl(
        struct_impl<OtherStorage, property_specs, method_specs,
                    L<OverLoads...>>&&
            other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage&&>)
        : storage_(std::move(other.storage_)),
          vtbl_(std::exchange(other.vtbl_, nullptr)) {}

    constexpr struct_impl(struct_impl&& other) noexcept(
        std::is_nothrow_constructible_v<StorageType, StorageType&&>)
        : storage_(std::move(other.storage_)),
          vtbl_(std::exchange(other.vtbl_, nullptr)) {}
    /// @}

    /// construct from a T
    /// @{
    template<typename T, typename = std::enable_if_t<not std::is_base_of_v<
                             struct_impl, std::decay_t<T>>>>
    constexpr struct_impl(T&& t) noexcept(
        detail::nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                                      decltype(t)>) {
      storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
      vtbl_ = &detail::struct_table_for<std::decay_t<T>,
                                        property_specs,
                                        method_specs>;
    }

    /// in place constructing a T
    template<typename T, typename... Args>
    constexpr struct_impl(traits::Id<T>, Args&&... args) noexcept(
        detail::nothrow_emplaceable_v<StorageType, T, decltype(args)...>) {
      storage_.template emplace<T>(std::forward<Args>(args)...);
      vtbl_ = &detail::struct_table_for<std::decay_t<T>,
                                        property_specs,
                                        method_specs>;
    }
    /// @}

    constexpr struct_impl& operator=(const struct_impl& other) noexcept(
        std::is_nothrow_copy_assignable_v<StorageType>) {
      vtbl_ = nullptr;
      storage_ = other.storage_;
      vtbl_ = other.vtbl_;
      return *this;
    }
    template<typename OtherStorage,
             typename = std::enable_if_t<
                 std::is_assignable_v<StorageType, const OtherStorage&>>>
    constexpr struct_impl& operator=(
        const struct_impl<OtherStorage, property_specs, method_specs,
                          L<OverLoads...>>&
            other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                         const OtherStorage&>) {
      vtbl_ = nullptr;
      storage_ = other.storage_;
      vtbl_ = other.vtbl_;
      return *this;
    }

    template<typename OtherStorage,
             typename = std::enable_if_t<
                 std::is_assignable_v<StorageType, OtherStorage&&>>>
    constexpr struct_impl& operator=(
        struct_impl<OtherStorage, property_specs, method_specs,
                    L<OverLoads...>>&&
            other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                         OtherStorage&&>) {
      vtbl_ = nullptr;
      storage_ = std::move(other.storage_);
      vtbl_ = std::exchange(other.vtbl_, nullptr);
      return *this;
    }
    constexpr struct_impl& operator=(struct_impl&& other) noexcept(
        std::is_nothrow_move_assignable_v<StorageType>) {
      vtbl_ = nullptr;
      storage_ = std::move(other.storage_);
      vtbl_ = std::exchange(other.vtbl_, nullptr);
      return *this;
    }

    template<typename T, typename = std::enable_if_t<not std::is_base_of_v<
                             struct_impl, std::decay_t<T>>>>
    constexpr struct_impl&
    operator=(T&& t) noexcept(detail::nothrow_emplaceable_v<
                              StorageType, std::decay_t<T>,
                              decltype(std::forward<T>(std::declval<T&&>()))>) {
      vtbl_ = nullptr;
      storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
      vtbl_ = &detail::struct_table_for<std::decay_t<T>,
                                        property_specs,
                                        method_specs>;
      return *this;
    }

    /**
     * call a non const method with arguments args.
     * @param args parameters for the method
     * @tparam MethodName name of the method
     * @tparam Args argument types
     * @returns the value returned by the method
     */
    template<typename MethodName, typename... Args>
    constexpr decltype(auto)
    call(Args&&... args) noexcept(nothrow_callable<MethodName, Args...>) {
      static_assert(
          std::is_invocable_v<const vtable_type, MethodName, void*, Args...>,
          "Attempting to call a method that does not exist!");
      assert(vtable());
      return (*vtable())(MethodName{},
                         storage_.data(),
                         std::forward<Args>(args)...);
    }

    /**
     * Call a const method with arguments args.
     * @param args parameters for the method
     * @tparam MethodName name of the method
     * @tparam Args argument types
     * @returns the value returned by the method
     */
    template<typename MethodName, typename... Args>
    constexpr decltype(auto) call(Args&&... args) const
        noexcept(nothrow_callable<MethodName, Args...>) {
      static_assert(std::is_invocable_v<const vtable_type,
                                        MethodName,
                                        const void*,
                                        Args...>,
                    "Attempting to call a method that does not exist!");
      assert(vtable());
      return (*vtable())(MethodName{},
                         storage_.data(),
                         std::forward<Args>(args)...);
    }

    /**
     * Set the value of a property.
     * @param value the new value of the property
     * @tparam Name the properties name
     * @returns boolean indicating if the new value was set (true) or not set
     * (false).
     */
    template<typename Name, typename = std::enable_if_t<not is_const<Name>>>
    constexpr bool
    set(const value_type_for<Name>& value) noexcept(is_nothrow<Name>) {
      return ptable()->set(Name{}, storage_.data(), value);
    }

    /**
     * Get the value of a property.
     * @tparam Name the properties name
     * @returns the properties value.
     */
    template<typename Name>
    constexpr value_type_for<Name> get() const noexcept(is_nothrow<Name>) {
      return ptable()->get(Name{}, storage_.data());
    }

    /**
     * returns true if an object is bound to the struct, i.e. the storage is not
     * empty, else false.
     */
    constexpr bool is_bound() const noexcept {
      return storage_.data() == nullptr;
    }

    /**
     * same as is_bound().
     */
    constexpr operator bool() const { return is_bound(); }

  private:
    constexpr const vtable_type* vtable() const noexcept { return vtbl_; }
    constexpr const ptable_type* ptable() const noexcept { return vtbl_; }

    using struct_table = detail::struct_table<property_specs, method_specs>;
    const struct_table* vtbl_{nullptr};
    StorageType storage_{};
  };
} // namespace detail

/// @addtogroup object Structs
/// Structs are the core class templates this library provides.
///
/// Structs provide methods and properties described in their lists of @ref
/// MethodSpec "MethodSpecs" and @ref PropertySpec "PropertySpecs".
///
/// Properties model member variables and are defined by @ref PropertySpec
/// "property specs". These specs describe the access (const/mutable), name,
/// and value type of the property.
///
/// A property of an Object with name **n** can be accessed by the
/// Structs set<**n**> and get<**n**> member functions. If name
/// injection is enabled, and the name **n** is created with the
/// @ref POLY_PROPERTY macro, the property can be accessed as if
/// it were a normal c++ member variable of that Object, i.e. ``auto v =
/// obj.n`` and ``obj.n = v`` instead of ``auto v = obj.get<n>()`` and
/// ``obj.set<n>(v)``.
///
/// Methods model member functions. They are invoked through an
/// Structs/Interfaces call member function.
///
/// @{

/// This class implements the polymorphic behaviour.
///
/// It holds objects in a storage of type StorageType, and provides the
/// properties and methods given by the list of PropertySpecs and MethodSpecs.
///
/// Struct instances can only be created and assigned to from types implementing
/// the properties and methods specified by the PropertySpecs and MethodSpecs,
/// and from other Structs with IDENTICAL PropertySpecs and MethodSpecs. Structs
/// cannot be created from Interfaces.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// poly::Storage concept.
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
/// @{
template<POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
         POLY_TYPE_LIST MethodSpecs>
using Struct =
    detail::struct_impl<StorageType, PropertySpecs, MethodSpecs,
                        typename detail::collapse_overloads<MethodSpecs>::type>;

/// A Reference is a non owning Struct and cheap to copy.
///
/// References can only be created and assigned to from types implementing the
/// properties and methods specified by the PropertySpecs and MethodSpecs, and
/// from other @ref basic_object "Structs" and References with IDENTICAL
/// PropertySpecs and MethodSpecs. References cannot be created from
/// Interfaces.
///
/// @warning  References do not own the bound object, just like normal C++
/// references. Bound objects must be alive for as long as methods and
/// properties of the Reference are accessed.
///
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
template<POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
using Reference = Struct<ref_storage, PropertySpecs, MethodSpecs>;

/// @}

} // namespace poly
#endif
