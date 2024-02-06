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
#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP

#include "poly/interface_method.hpp"
#include "poly/interface_property.hpp"
#include "poly/object.hpp"
#include "poly/traits.hpp"

namespace poly {
namespace detail {

  /// wrapper for an object_table.
  /// @{
  template<POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
  struct InterfaceTable;

  template<template<typename...> typename L, POLY_PROP_SPEC... PropertySpecs,
           POLY_METHOD_SPEC... MethodSpecs>
  struct InterfaceTable<L<PropertySpecs...>, L<MethodSpecs...>>
      : public InterfaceVTableEntry<MethodSpecs>...,
        InterfacePTableEntry<PropertySpecs>... {
  public:
    using InterfaceVTableEntry<MethodSpecs>::operator()...;
    template<typename MethodName, typename... Args>
    static constexpr bool nothrow_callable =
        noexcept((*std::declval<const VTable<MethodSpecs...>*>())(
            MethodName{}, std::declval<void*>(), std::declval<Args>()...));

    template<typename Name>
    using spec_for = typename spec_by_name<Name, PropertySpecs...>::type;
    template<typename Name>
    using value_type_for = value_type_t<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

    constexpr InterfaceTable(const InterfaceTable&) noexcept = default;
    constexpr InterfaceTable(InterfaceTable&&) noexcept = default;
    constexpr InterfaceTable&
    operator=(const InterfaceTable&) noexcept = default;
    constexpr InterfaceTable& operator=(InterfaceTable&&) noexcept = default;

    // construct from other InterfaceTable featureing a super set of
    // properties and methods
    template<typename Ps, typename Ms>
    InterfaceTable(const InterfaceTable<Ps, Ms>& other)
        : InterfaceVTableEntry<MethodSpecs>(
              static_cast<const InterfaceVTableEntry<MethodSpecs>&>(other))...,
          InterfacePTableEntry<PropertySpecs>(
              static_cast<const InterfacePTableEntry<PropertySpecs>&>(
                  other))...,
          table_(other.table_) {}

    template<typename Ps, typename Ms>
    InterfaceTable(const object_table<Ps, Ms>* table)
        // note: aggregate initialization of base classes on purpose->
        // direclty initialize offset
        : InterfaceVTableEntry<MethodSpecs>{object_table<Ps, Ms>::method_offset(
              traits::Id<MethodSpecs>{})}...,
          InterfacePTableEntry<PropertySpecs>{
              object_table<Ps, Ms>::property_offset(
                  traits::Id<PropertySpecs>{})}...,
          table_(table) {
      static_assert(
          poly::detail::list_size<Ms>::value <= poly::config::max_method_count,
          "Error while creating an Interface from an Object: the number of "
          "methods in the Object exceeds POLY_MAX_METHOD_COUNT . Adjust the "
          "POLY_MAX_METHOD_COUNT macro to reflect the maximum number of "
          "methods accurately before including ANY poly header.");
      static_assert(
          poly::detail::list_size<Ps>::value <= poly::config::max_property_count,
          "Error while creating an Interface from an Object: the number of "
          "properties in the Object exceeds POLY_MAX_PROPERTY_COUNT. Adjust "
          "the macro to reflect the maximum number of porperties accurately "
          "before including ANY poly header.");
    }

    template<typename MethodName, typename... Args>
    decltype(auto)
    call(void* obj,
         Args&&... args) noexcept(nothrow_callable<MethodName, Args&&...>) {
      assert(obj);
      assert(table_);
      return (*this)(MethodName{}, table_, obj, std::forward<Args>(args)...);
    }

    template<typename MethodName, typename... Args>
    decltype(auto) call(const void* obj, Args&&... args) const
        noexcept(nothrow_callable<MethodName, Args&&...>) {
      assert(obj);
      assert(table_);
      return (*this)(MethodName{}, table_, obj, std::forward<Args>(args)...);
    }

    template<typename Name>
    bool set(void* obj,
             const value_type_for<Name>& value) noexcept(is_nothrow<Name>) {
      assert(obj);
      assert(table_);
      return this->set(Name{}, table_, obj, value);
    }

    template<typename Name>
    value_type_for<Name> get(const void* obj) noexcept(is_nothrow<Name>) {
      assert(obj);
      assert(table_);
      return this->get(Name{}, table_, obj);
    }

  private:
    const void* table_{
        nullptr}; ///< points to original object_table this is created with
  };
  /// @}

  template<POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
           POLY_TYPE_LIST MethodSpecs, POLY_TYPE_LIST OverLoads>
  struct basic_interface_impl;

  template<POLY_STORAGE StorageType, template<typename...> typename List,
           typename... PropertySpecs, typename... MethodSpecs,
           typename... Overloads>
  struct POLY_EMPTY_BASE
      basic_interface_impl<StorageType, List<PropertySpecs...>,
                           List<MethodSpecs...>, List<Overloads...>>
      : detail::MethodInjector<
            Overloads,
            basic_interface_impl<StorageType, List<PropertySpecs...>,
                                 List<MethodSpecs...>, List<Overloads...>>>...,
        detail::property_injector_for_t<
            PropertySpecs,
            basic_interface_impl<StorageType, List<PropertySpecs...>,
                                 List<MethodSpecs...>, List<Overloads...>>>... {
    using table_type =
        detail::InterfaceTable<List<PropertySpecs...>, List<MethodSpecs...>>;

  public:
    template<typename MethodName, typename... Args>
    static constexpr bool nothrow_callable =
        table_type::template nothrow_callable<MethodName, Args...>;
    template<typename Name>
    using spec_for = typename table_type::template spec_for<Name>;
    template<typename Name>
    using value_type_for = value_type_t<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
    template<typename Name>
    static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

    using property_specs = List<PropertySpecs...>;
    using method_specs = List<MethodSpecs...>;

    template<typename S, typename Ps, typename Ms>
    basic_interface_impl(basic_object<S, Ps, Ms>& obj)
        : storage_(obj.storage_), vtbl_(obj.vtbl_) {}

    template<typename S, typename Ps, typename Ms,
             typename = std::enable_if_t<
                 std::is_constructible_v<StorageType, const S&>>>
    basic_interface_impl(const basic_object<S, Ps, Ms>& obj) noexcept(
        std::is_nothrow_constructible_v<StorageType, const S&>)
        : storage_(obj.storage_), vtbl_(obj.vtbl_) {}

    template<
        typename S, typename Ps, typename Ms,
        typename = std::enable_if_t<std::is_constructible_v<StorageType, S&&>>>
    basic_interface_impl(basic_object<S, Ps, Ms>&& obj) noexcept(
        std::is_nothrow_constructible_v<StorageType, S&&>)
        : storage_(std::move(obj.storage_)), vtbl_(obj.vtbl_) {}

    basic_interface_impl(const basic_interface_impl& other) noexcept(
        std::is_nothrow_copy_constructible_v<StorageType>)
        : storage_(other.storage_), vtbl_(other.vtbl_) {}

    basic_interface_impl(basic_interface_impl&& other) noexcept(
        std::is_nothrow_move_constructible_v<StorageType>)
        : storage_(std::move(other.storage_)), vtbl_(other.vtbl_) {}

    template<typename S, typename Ps, typename Ms, typename Os,
             typename = std::enable_if_t<
                 std::is_constructible_v<StorageType, const S&>>>
    basic_interface_impl(
        const basic_interface_impl<S, Ps, Ms, Os>&
            other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            const S&>)
        : storage_(other.storage_), vtbl_(other.vtbl_) {}

    template<
        typename S, typename Ps, typename Ms, typename Os,
        typename = std::enable_if_t<std::is_constructible_v<StorageType, S&&>>>
    basic_interface_impl(basic_interface_impl<S, Ps, Ms, Os>&& other) noexcept(
        std::is_nothrow_constructible_v<StorageType, S&&>)
        : storage_(std::move(other.storage_)), vtbl_(other.vtbl_) {}

    template<typename S = StorageType,
             typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
    basic_interface_impl& operator=(const basic_interface_impl& other) noexcept(
        std::is_nothrow_constructible_v<S>) {
      storage_ = other.storage_;
      vtbl_ = other.vtbl_;
      return *this;
    }

    basic_interface_impl& operator=(basic_interface_impl&& other) noexcept(
        std::is_nothrow_move_constructible_v<StorageType>) {
      storage_ = std::move(other.storage_);
      vtbl_ = other.vtbl_;
      return *this;
    }
    template<
        typename S, typename Ps, typename Ms, typename Os,
        typename = std::enable_if_t<std::is_assignable_v<StorageType, S&&>>>
    basic_interface_impl&
    operator=(basic_interface_impl<S, Ps, Ms, Os>&& other) noexcept(
        std::is_nothrow_assignable_v<StorageType, S&&>) {
      storage_ = std::move(other.storage_);
      vtbl_ = other.vtbl_;
      return *this;
    }
    template<typename S, typename Ps, typename Ms, typename Os,
             typename =
                 std::enable_if_t<std::is_assignable_v<StorageType, const S&>>>
    basic_interface_impl&
    operator=(const basic_interface_impl<S, Ps, Ms, Os>& other) noexcept(
        std::is_nothrow_assignable_v<StorageType, const S&>) {
      storage_ = other.storage_;
      vtbl_ = other.vtbl_;
      return *this;
    }

    template<typename MethodName, typename... Args>
    decltype(auto)
    call(Args&&... args) noexcept(nothrow_callable<MethodName, Args&&...>) {
      return vtbl_.template call<MethodName>(storage_.data(),
                                             std::forward<Args>(args)...);
    }

    template<typename MethodName, typename... Args>
    decltype(auto) call(Args&&... args) const
        noexcept(nothrow_callable<MethodName, Args&&...>) {
      return vtbl_.template call<MethodName>(storage_.data(),
                                             std::forward<Args>(args)...);
    }

    template<typename Name>
    bool set(Name,
             const value_type_for<Name>& value) noexcept(is_nothrow<Name>) {
      static_assert(
          contains_v<transform_t<property_specs, traits::property_name_t>,
                     Name> &&
              not is_const<Name>,
          "Name not specified in this interface.");
      return vtbl_.template set<Name>(storage_.data(), value);
    }

    template<typename Name>
    value_type_for<Name> get(Name) noexcept(is_nothrow<Name>) {
      static_assert(contains_v<property_specs, Name>,
                    "Name not specified in this interface.");
      return vtbl_.template get<Name>(storage_.data());
    }

  private:
    StorageType storage_;
    table_type vtbl_;
  };
} // namespace detail
/// @addtogroup interface Interface
/// An Interface provides a subset of an Object.
/// @{

/// An Interface provides a subset of an Object. They can constrain existing
/// Object types and are more flexible, as they can be created from any Object
/// or Interface providing a super set of the methods and properties specified
/// by the MethodSpecs and PropertySpecs.
///
/// Interfaces can only be created from Objects/Interfaces providing a super
/// set of properties and methods, and not directly from types implementing
/// the specified methods and properties.
///
/// @note Interfaces do not generate extra vtables and property tables, but
/// adapt the tables from the Object the Interface is created from. This
/// necessitates extra stack space for each instance, and disables constexpr
/// use because of pointer arithmetic and casting.
///
/// @warning If an Object with more than 255 properties is bound to an
/// Interface with at least one property, the macro POLY_MAX_PROPERTY_COUNT
/// must be set accordingly before including any poly header.
///
/// @warning If an Object with more than 255 methods is bound to an Interface
/// with at least one method, the macro POLY_MAX_METHOD_COUNT must be set
/// accordingly before including any poly header.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// @ref Storage "poly::Storage" concept.
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
/// @{
/// @}
template<POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
         POLY_TYPE_LIST MethodSpecs>
using basic_interface = detail::basic_interface_impl<
    StorageType, PropertySpecs, MethodSpecs,
    typename detail::collapse_overloads<MethodSpecs>::type>;
/// An non owning Interface. Can be used to pass Objects, References and other
/// Interfaces.
///
/// @warning The same lifetime restrictions apply as with @ref Reference
/// "References".
template<typename PropertySpecs, typename MethodSpecs>
using Interface = basic_interface<ref_storage, PropertySpecs, MethodSpecs>;

/// @}
} // namespace poly
#endif
