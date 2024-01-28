#ifndef POLY_PROPERTY_HPP
#define POLY_PROPERTY_HPP
#include "poly/always_false.hpp"
#include "poly/config.hpp"
#include <type_traits>

namespace poly {
/// @addtogroup PropertySpec
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
/// interface::set<PropertyName>(value) returns the result of calling check().
///
/// @note when using the assignment operator on injected properties,instead of
/// set(), the value is set as described above, but return value of check()
/// cannot be retrieved by the caller.
///
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename PropertyName, typename T, typename Type,
          typename = std::enable_if_t<detail::always_false<T>>>
bool check(PropertyName, const T &t, const Type &new_value);
/// @}
} // namespace poly

#if POLY_USE_MACROS
#if POLY_USE_PROPERTY_INJECTOR
#define POLY_PROPERTY_IMPL(name)                                               \
  struct name {                                                                \
    template <typename Self, typename Spec> struct injector {                  \
      poly::detail::InjectedProperty<                                          \
          Self, injector<Self, Spec>, poly::property_name_t<Spec>,             \
          poly::value_type_t<Spec>, poly::is_const_property_v<Spec>,           \
          poly::is_nothrow_property_v<Spec>>                                   \
          name;                                                                \
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

#define POLY_PROPERTY(name)                                                    \
  POLY_PROPERTY_IMPL(name)                                                     \
  POLY_ACCESS_IMPL(name)
#endif
#endif
