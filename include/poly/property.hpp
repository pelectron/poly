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
#ifndef POLY_PROPERTY_HPP
#define POLY_PROPERTY_HPP
#include "poly/always_false.hpp"
#include "poly/config.hpp"
#include <type_traits>

namespace poly {
/// @addtogroup property_extension Property Extension
/// @ref PropertySpecs "PropertySpecs" for an arbitrary type T are implemented
/// by defining the functions set(), get(), and optionally check().
///
/// These functions must be locatable through "argument dependent lookup" (ADL),
/// that is, they should be defined in the same namespace as the property name,
/// or the same namespace as the T to be extended with the property. In general,
/// if the T is a type owned by the user, the functions should be in the same
/// namespace as T. For third party types, such as standard library containers,
/// they should be defined in the same namespace as the property name.
///
/// To enable name injection, the @ref POLY_PROPERTY macro has to be used to
/// define the property name.
/// @{

/// getter for the PropertySpec
/// '[const]PropertyName(Type)[noexcept]'.
///
/// This function needs to be defined for a T to implement a specific
/// PropertySpec. For nothrow PropertySpecs, the get() function must be
/// noexcept.
///
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename Type, typename PropertyName, typename T,
          typename = std::enable_if_t<detail::always_false<T>>>
Type get(PropertyName, const T &t);

/// setter for the PropertySpec
/// 'PropertyName(Type)[noexcept]'.
///
/// This function needs to be defined for a T to implement a specific
/// PropertySpec. For nothrow PropertySpecs, the set() function must be
/// noexcept.
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename PropertyName, typename T, typename Type,
          typename = std::enable_if_t<detail::always_false<T>>>
void set(PropertyName, T &t, const Type &value);

/// optional checker for the PropertySpec
/// '[const]PropertyName(Type)[noexcept]'.
///
/// This function can be defined for enable validation before setting properties
/// on a T. If the function returns false, the value is not set, i.e
/// set(PropertyName,T&,const Type&) is not called. Calling
/// Object::set<PropertyName>(value) returns the result of calling check().
///
/// @note when using the assignment operator on injected properties,instead of
/// Object::set<PropertyName>(), the value is set as described above, but return
/// value of check() cannot be retrieved by the caller.
///
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename PropertyName, typename T, typename Type,
          typename = std::enable_if_t<detail::always_false<T>>>
bool check(PropertyName, const T &t, const Type &new_value);

/// @def POLY_PROPERTY(Name)
/// Defines a property name Name.
///
/// If property injection is disabled, this macro will simply expand to
///
/// ```
/// struct Name{};
/// ```
///
/// If property injection is enabled, an inner template called injector will
/// defined in name. The injector contains the member "Name", which delegates
/// assignment and conversion to the Object that contains the property.
/// Simplified, it looks like this:
///
/// ```{cpp}
/// struct Name{
///   struct Injector{
///     InjectedProperty Name;
///   };
/// };
/// ```
///
/// Additionally, if not disabled, default property access functions are
/// generated as follows:
///
/// ```
/// template<typename T, typename ValueType>
/// void set(Name, T& t, const ValueType& v) {
///   t.Name = v;
/// }
///
/// template<typename T, typename ValueType>
/// ValueType get(Name, const T& t) {
///   return t.Name;
/// }
/// ```

/// @}
} // namespace poly

#if POLY_USE_MACROS

#define POLY_PROPERTIES(...) poly::type_list<__VA_ARGS__>

#define POLY_PROPERTY(Name)                                                    \
  POLY_PROPERTY_IMPL(Name)                                                     \
  POLY_ACCESS_IMPL(Name)

#if POLY_USE_PROPERTY_INJECTOR

#define POLY_PROPERTY_IMPL(name)                                               \
  struct POLY_EMPTY_BASE name {                                                \
                                                                               \
    template <typename Self, POLY_PROP_SPEC Spec> struct injector {            \
      using InjectedProperty = poly::detail::InjectedProperty<                 \
          Self, injector<Self, Spec>, poly::property_name_t<Spec>,             \
          poly::value_type_t<Spec>, poly::is_const_property_v<Spec>,           \
          poly::is_nothrow_property_v<Spec>>;                                  \
                                                                               \
      POLY_NO_UNIQUE_ADDRESS InjectedProperty name;                            \
    };                                                                         \
  };
#else
#define POLY_PROPERTY_IMPL(name)                                               \
  struct name {};
#endif

#if POLY_USE_DEFAULT_PROPERTY_ACCESS

#define POLY_ACCESS_IMPL(name)                                                 \
                                                                               \
  template <typename T>                                                        \
  auto get(name, const T &t) noexcept(                                         \
      std::is_nothrow_copy_constructible_v<                                    \
          decltype(std::declval<const T &>().name)>) {                         \
    return t.name;                                                             \
  }                                                                            \
                                                                               \
  template <typename T, typename Type>                                         \
  void set(name, T &t, const Type &value) noexcept(                            \
      std::is_nothrow_assignable_v<decltype(std::declval<T &>().name),         \
                                   const Type &>) {                            \
    t.name = value;                                                            \
  }
#else
#define POLY_ACCESS_IMPL(name)
#endif
#endif

#endif
