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
#ifndef POLY_TRAITS_HPP
#define POLY_TRAITS_HPP

#include "poly/type_list.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace poly {
template<typename... Ts>
class variant_storage;

namespace traits {

  template<typename T>
  inline constexpr bool is_moveable_v =
      std::is_move_assignable_v<T> and std::is_move_constructible_v<T>;

  class cls {
  public:
    template<typename... A>
    cls(A... a) : data_{1} {
      ((void)a, ...);
      (void)data_;
    }

  private:
    char data_{0};
  };

  template<typename T>
  struct is_storage;

  template<typename T, typename = void>
  struct is_storage_impl : std::false_type {};

  template<typename T, typename = void>
  struct has_single_lvalue_ref_emplace : std::false_type {};
  template<typename T>
  struct has_single_lvalue_ref_emplace<
      T,
      std::void_t<decltype(T{}.template emplace<cls>(std::declval<cls&>()))>> {
    static constexpr bool value =
        std::is_same_v<cls*, decltype(T{}.template emplace<cls>(
                                 std::declval<cls&>()))>;
  };
  template<typename T, typename = void>
  struct has_mutli_value_emplace : std::false_type {};
  template<typename T>
  struct has_mutli_value_emplace<
      T, std::void_t<decltype(T{}.template emplace<cls>(1, 2, 3, 4))>> {
    static constexpr bool value =
        std::is_same_v<cls*, decltype(T{}.template emplace<cls>(1, 2, 3, 4))>;
  };
  template<typename T>
  struct is_storage_impl<
      T,
      std::void_t<decltype(std::declval<T>() = std::move(std::declval<T&&>())),
                  decltype(std::declval<T>().data()),
                  decltype(std::declval<const T>().data())>> {

    static constexpr bool has_data =
        std::is_same_v<void*, decltype(std::declval<T>().data())>;
    static constexpr bool has_const_data =
        std::is_same_v<const void*, decltype(std::declval<const T>().data())>;
    static constexpr bool value =
        has_data && has_const_data && is_moveable_v<T> &&
        (has_single_lvalue_ref_emplace<T, void>::value or
         has_mutli_value_emplace<T, void>::value);
  };

  template<typename T>
  struct is_storage
      : public std::conditional_t<is_storage_impl<T, void>::value,
                                  std::true_type, std::false_type> {};

  template<typename... Ts>
  struct is_storage<::poly::variant_storage<Ts...>> : std::true_type {};

  template<typename T>
  struct is_type_list : std::false_type {};

  template<template<typename...> typename L, typename... Ts>
  struct is_type_list<L<Ts...>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_storage_v = is_storage<T>::value;

  /// simply utility "identity" type
  template<typename T>
  struct Id {
    using type = T;
  };

  /// @{

  /// gets a function types return type.
  /// @{
  template<typename Sig>
  struct func_return_type;
  template<typename Ret, typename... Args>
  struct func_return_type<Ret(Args...)> {
    using type = Ret;
  };
  template<typename Ret, typename... Args>
  struct func_return_type<Ret(Args...) const> {
    using type = Ret;
  };
  template<typename Ret, typename... Args>
  struct func_return_type<Ret(Args...) noexcept> {
    using type = Ret;
  };
  template<typename Ret, typename... Args>
  struct func_return_type<Ret(Args...) const noexcept> {
    using type = Ret;
  };
  /// @}

  /// gets a function types first argument, or void if the function has no
  /// arguments.
  /// @{
  template<typename Sig>
  struct func_first_arg;
  template<typename Ret, typename... Args>
  struct func_first_arg<Ret(Args...)> {
    using type = at_t<type_list<Args...>, 0>;
  };
  template<typename Ret, typename... Args>
  struct func_first_arg<Ret(Args...) const> {
    using type = at_t<type_list<Args...>, 0>;
  };
  template<typename Ret, typename... Args>
  struct func_first_arg<Ret(Args...) noexcept> {
    using type = at_t<type_list<Args...>, 0>;
  };
  template<typename Ret, typename... Args>
  struct func_first_arg<Ret(Args...) const noexcept> {
    using type = at_t<type_list<Args...>, 0>;
  };
  template<typename Ret>
  struct func_first_arg<Ret() noexcept> {
    using type = void;
  };
  template<typename Ret>
  struct func_first_arg<Ret() const noexcept> {
    using type = void;
  };
  template<typename Ret>
  struct func_first_arg<Ret()> {
    using type = void;
  };
  template<typename Ret>
  struct func_first_arg<Ret() const> {
    using type = void;
  };
  /// @}

  /// evaluates to true if the function type is const
  /// @{
  template<typename Sig>
  struct func_is_const;
  template<typename Ret, typename... Args>
  struct func_is_const<Ret(Args...)> : std::false_type {};
  template<typename Ret, typename... Args>
  struct func_is_const<Ret(Args...) const> : std::true_type {};
  template<typename Ret, typename... Args>
  struct func_is_const<Ret(Args...) noexcept> : std::false_type {};
  template<typename Ret, typename... Args>
  struct func_is_const<Ret(Args...) const noexcept> : std::true_type {};
  /// @}

  /// evaluates to true if the function type is noexcept
  /// @{
  template<typename Sig>
  struct func_is_noexcept;
  template<typename Ret, typename... Args>
  struct func_is_noexcept<Ret(Args...)> : std::false_type {};
  template<typename Ret, typename... Args>
  struct func_is_noexcept<Ret(Args...) const> : std::false_type {};
  template<typename Ret, typename... Args>
  struct func_is_noexcept<Ret(Args...) noexcept> : std::true_type {};
  template<typename Ret, typename... Args>
  struct func_is_noexcept<Ret(Args...) const noexcept> : std::true_type {};
  /// @}

  /// evaluates to true if the function type is noexcept
  /// @{
  template<typename Sig>
  struct func_args;
  template<typename Ret, typename... Args>
  struct func_args<Ret(Args...)> {
    using type = type_list<Args...>;
  };
  template<typename Ret, typename... Args>
  struct func_args<Ret(Args...) const> {
    using type = type_list<Args...>;
  };
  template<typename Ret, typename... Args>
  struct func_args<Ret(Args...) noexcept> {
    using type = type_list<Args...>;
  };
  template<typename Ret, typename... Args>
  struct func_args<Ret(Args...) const noexcept> {
    using type = type_list<Args...>;
  };
  /// @}

  /// evaluates to true if F is incovable with the signature Sig
  /// @{
  template<typename Sig, typename F>
  struct invocable_r;
  template<typename Ret, typename... Args, typename F>
  struct invocable_r<Ret(Args...), F> : std::is_invocable_r<Ret, F, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable_r<Ret(Args...) const, F>
      : std::is_invocable_r<Ret, std::add_const_t<F>, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable_r<Ret(Args...) noexcept, F>
      : std::is_nothrow_invocable_r<Ret, F, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable_r<Ret(Args...) const noexcept, F>
      : std::is_nothrow_invocable_r<Ret, std::add_const_t<F>, Args...> {};

  template<typename Sig, typename F>
  struct invocable;
  template<typename Ret, typename... Args, typename F>
  struct invocable<Ret(Args...), F> : std::is_invocable<F, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable<Ret(Args...) const, F>
      : std::is_invocable<std::add_const_t<F>, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable<Ret(Args...) noexcept, F>
      : std::is_nothrow_invocable<F, Args...> {};
  template<typename Ret, typename... Args, typename F>
  struct invocable<Ret(Args...) const noexcept, F>
      : std::is_nothrow_invocable<std::add_const_t<F>, Args...> {};
  /// @}
  /// @}

  template<typename T>
  struct is_property_spec : std::false_type {};

  /// get the value type of a PropertySpec
  template<typename PropertySpec>
  using value_type_t =
      std::enable_if_t<is_property_spec<PropertySpec>::value,
                       typename func_first_arg<PropertySpec>::type>;

  /// check if a PropertySpec is const
  template<typename PropertySpec>
  inline constexpr bool is_const_v = std::enable_if_t<
      is_property_spec<PropertySpec>::value,
      std::is_const<typename func_return_type<PropertySpec>::type>>::value;

  template<typename T>
  struct is_method_spec : std::false_type {};

  /// get the return type of a MethodSpec
  template<typename MethodSpec>
  using return_type_t =
      std::enable_if_t<is_method_spec<MethodSpec>::value,
                       typename func_return_type<MethodSpec>::type>;

  template<typename R, typename M, typename... Args>
  struct is_method_spec<R(M, Args...)>
      : std::conditional_t<std::is_empty_v<M>, std::true_type,
                           std::false_type> {};
  template<typename R, typename M, typename... Args>
  struct is_method_spec<R(M, Args...) const>
      : std::conditional_t<std::is_empty_v<M>, std::true_type,
                           std::false_type> {};
  template<typename R, typename M, typename... Args>
  struct is_method_spec<R(M, Args...) noexcept>
      : std::conditional_t<std::is_empty_v<M>, std::true_type,
                           std::false_type> {};
  template<typename R, typename M, typename... Args>
  struct is_method_spec<R(M, Args...) const noexcept>
      : std::conditional_t<std::is_empty_v<M>, std::true_type,
                           std::false_type> {};

  template<typename T>
  inline constexpr bool is_method_spec_v = is_method_spec<T>::value;

  template<typename Name, typename Type>
  struct is_property_spec<Name(Type)>
      : std::conditional_t<std::is_empty_v<Name>, std::true_type,
                           std::false_type> {};
  template<typename Name, typename Type>
  struct is_property_spec<const Name(Type)>
      : std::conditional_t<std::is_empty_v<Name>, std::true_type,
                           std::false_type> {};
  template<typename Name, typename Type>
  struct is_property_spec<Name(Type) noexcept>
      : std::conditional_t<std::is_empty_v<Name>, std::true_type,
                           std::false_type> {};
  template<typename Name, typename Type>
  struct is_property_spec<const Name(Type) noexcept>
      : std::conditional_t<std::is_empty_v<Name>, std::true_type,
                           std::false_type> {};

  template<typename T>
  inline constexpr bool is_property_spec_v = is_property_spec<T>::value;

  template<typename PropertySpec>
  using property_name_t =
      std::enable_if_t<is_property_spec_v<PropertySpec>,
                       std::remove_const_t<typename traits::func_return_type<
                           PropertySpec>::type>>;

  template<typename PropertySpec>
  inline constexpr bool is_nothrow_property_v =
      std::enable_if_t<is_property_spec_v<PropertySpec>,
                       func_is_noexcept<PropertySpec>>::value;

  template<auto value>
  using smallest_uint_to_contain = std::conditional_t<
      std::size_t(value) <= 0xFFu, std::uint8_t,
      std::conditional_t<std::size_t(value) <= 0xFFFFu, std::uint16_t,
                         std::conditional_t<std::size_t(value) <= 0xFFFFFFFFu,
                                            std::uint32_t, std::uint64_t>>>;

  /// evaluates to true if F is invocable with the signature Sig.
  template<typename Sig, typename F>
  inline constexpr bool is_invocable_v =
      ::poly::traits::invocable_r<Sig, F>::value;
  template<typename T, typename PropertySpec>
  std::false_type has_validator(...);
  template<typename T, typename PropertySpec>
  std::true_type has_validator(decltype(sizeof(
      check(std::remove_const_t<
                typename traits::func_return_type<PropertySpec>::type>{},
            std::declval<const T&>(),
            std::declval<const value_type_t<PropertySpec>&>()))));

} // namespace traits

/// @addtogroup PropertySpec PropertySpec Concept
/// A PropertySpec defines the name, value type and access of a property.
///
/// It is a function type with the following allowed patterns:
/// - \verbatim PropertyName(ValueType) \endverbatim a read/write property
/// with name PropertyName and value type ValueType.
/// - \verbatim const PropertyName(ValueType) \endverbatim a read only
/// property
/// - \verbatim PropertyName(ValueType) noexcept \endverbatim a read/write
/// property with noexcept access
/// - \verbatim const PropertyName(ValueType) noexcept \endverbatim a read
/// only property with noexcept access
///
/// The PropertyName is a tag type and does not contain any data. It is simply
/// used to tag the @ref property_extension "property extension functions".
/// PropertyName must be completely defined. Can either be defined as an empty
/// struct, i.e. ```struct PropertyName{};```, or with the @ref POLY_PROPERTY
/// macro, i.e. ```POLY_PROPERTY(PropertyName)```. Using the macro will use
/// name injection if enabled.
///
/// ValueType is the actual type used to get (and optionally set) the
/// property.
///
/// Example:
///
/// A read only property with the name "size" and type size_t should be added
/// to an Object.
///
/// Defining the name is done as described above:
/// ```
/// POLY_PROPERTY(size)
/// // or struct size{};
/// ```
/// The corresponding signature can be declared as follows, noexcept is added
/// because size_t is nothrow copyable and no checkers (see poly::check()) are
/// involved.
/// ```
/// using SizeSpec = const size(size_t) noexcept;
/// ```
///
/// Objects using SizeSpec now provide the size property, accessible with
/// ```
/// obj.get<size>()
/// ```
/// and
/// ```
/// obj.set<size>(size_t{...})
/// ```
///
/// If name injection is enabled (default), size can also be accessed with
/// ```
/// obj.size
/// ```
///
/// Several type traits are available to access information about
/// PropertySpecs.
///
/// - poly::is_property_spec_v<Spec> : true if Spec is a PropertySpec.
/// - poly::is_nothrow_property_v<Spec> : true if Spec is specified noexcept.
/// - poly::is_const_property_v<Spec> : true if Spec is specified const.
/// - poly::value_type_t<Spec> : provides the ValueType of a Spec.
/// - poly::property_name_t<Spec>: provides the PropertyName of Spec.
/// @{

/// evaluates to true if T is a valid PropertySpec.
template<typename T>
inline constexpr bool is_property_spec_v = traits::is_property_spec_v<T>;

/// get the value type of a PropertySpec
template<typename PropertySpec>
using value_type_t = traits::value_type_t<PropertySpec>;

/// get the name of a PropertySpec
template<typename PropertySpec>
using property_name_t = traits::property_name_t<PropertySpec>;

/// evaluates to true if a PropertySpec is specified noexcept
template<typename PropertySpec>
inline constexpr bool is_nothrow_property_v =
    traits::is_nothrow_property_v<PropertySpec>;

/// evaluates to true if valid check() is defined for T and the PropertySpec
template<typename T, typename PropertySpec>
inline constexpr bool has_validator_v =
    decltype(traits::has_validator<T, PropertySpec>(0))::value;

/// evaluates to true if a PropertySpec is const, i.e. read only.
template<typename PropertySpec>
inline constexpr bool is_const_property_v = traits::is_const_v<PropertySpec>;

/// @}

/// @addtogroup MethodSpec MethodSpec Concept
/// A MethodSpec defines the return type, name, and arguments of a method.
///
/// A method, in the context of poly, is a member function to be called on an
/// Object. It is a function signature type with the allowed patterns:
/// - \verbatim ReturnType(MethodName, ArgumentTypes...) \endverbatim
/// specifies a method with return type ReturnType, name MethodName, and
/// arguments of type ArgumentTypes.
/// - \verbatim ReturnType(MethodName, ArgumentTypes...) const \endverbatim
/// specifies a const method, i.e. the Object it is called on is not modified.
/// - \verbatim ReturnType(MethodName, ArgumentTypes...) noexcept \endverbatim
/// specifies a non throwing method.
/// - \verbatim ReturnType(MethodName, ArgumentTypes...) const noexcept
/// \endverbatim specifies a non throwing const method.
///
/// The MethodName is a tag type and does not contain any data. It is simply
/// used to tag @ref method_extension "extend" to enable methods with
/// identical arguments and overloaded methods.
///
/// @{

/// evaluates to true if T is a valid MethodSpec.
template<typename T>
inline constexpr bool is_method_spec_v = traits::is_method_spec_v<T>;

/// get the return type of a MethodSpec.
template<typename MethodSpec>
using return_type_t = traits::return_type_t<MethodSpec>;

/// get the name of a MethodSpec
template<typename MethodSpec>
using method_name_t = typename traits::func_first_arg<MethodSpec>::type;

/// evaluates to true if MethodSpec if specified noexcept
template<typename MethodSpec>
inline constexpr bool is_nothrow_method_v =
    traits::func_is_noexcept<MethodSpec>::value;

/// evaluates to true if MethodSpec if specified const
template<typename MethodSpec>
inline constexpr bool is_const_method_v =
    traits::func_is_const<MethodSpec>::value;

/// @}

/// @addtogroup Storage Storage Concept
/// Storages represent a type erased container.
///
/// Objects of arbitrary types can be emplaced, that is they are
/// stored owning or non-owning, in a Storage object.
///
/// Objects are emplaced into a Storage with its emplace() template method.
/// The first template parameter specifies the type of the emplaced object.
/// The rest, if there are any, must be deductible by the compiler. emplace()
/// returns a reference to the emplaced object. Non-owning Storages take
/// exactly one (possibly const in case of non modifiable storage) lvalue
/// reference parameter of the type to be emplaced. Owning storages take an
/// arbitrary amount of arguments used for constructing the type to be
/// emplaced.
///
/// Upon emplacement, the stored object can be accessed as a (const) void
/// pointer through the Storages data() method. Calling data() on an empty
/// Storage returns nullptr.
///
/// Emplacing a new object into a Storage already containing an object will
/// destroy the currently emplaced object and emplace the new object.
///
/// Destroying a Storage through its destructor will destroy the emplaced
/// object appropriately, i.e. call the destructor of the emplaced object in
/// case of an owning Storage, or simply reset its internal reference/pointer
/// in the case of non-owning Storage.
///
/// In addition, a Storage is at least moveable, i.e. move constructible
/// and move assignable, as well as default constructible. A default
/// constructed Storage is empty. Moved from Storages are empty and destroy
/// their contained object before the move operation returns, i.e. before the
/// move constructor/assignment operator returns to the caller.
///
/// The minimal interface is depicted here:
///
/// ```
/// struct minimal_storage{
///   minimal_storage();
///   minimal_storage(minimal_storage&&);
///   minimal_storage& operator=(minimal_storage&&);
///
///   void* data();
///   const void* data() const;
///
///   // typical owning storage emplace
///   template<typename T,typename...Args>
///   T& emplace(Args&&args);
///   // or below for non owning storage
///   template<typename T>
///   T& emplace(T& t);
/// };
/// ```
/// @{

/// evaluates to true if T satisfies the @ref Storage concept.
template<typename T>
inline constexpr bool is_storage_v = traits::is_storage_v<T>;
/// @}

#if __cplusplus > 201703L
// enabled if c++ std > c++17
/** @concept Storage
 * @see poly::is_storage_v
 */
template<typename T>
concept Storage = ::poly::is_storage_v<T>;

/** @concept MethodSpecification
 * @see poly::is_method_spec_v
 */
template<typename T>
concept MethodSpecification = ::poly::is_method_spec_v<T>;

template<typename T>
concept PropertySpecification = ::poly::is_property_spec_v<T>;

#endif
} // namespace poly

#endif
