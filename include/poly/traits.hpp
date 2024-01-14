#ifndef POLY_TRAITS_HPP
#define POLY_TRAITS_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include "poly/type_list.hpp"

#if __cplusplus > 201703L
// include if c++ std > c++17
#include <concepts>
#define POLY_STORAGE poly::traits::Storage
#else
#define POLY_STORAGE typename
#endif

namespace poly {
namespace traits {

template <typename T>
inline constexpr bool is_moveable_v =
    std::is_move_assignable_v<T> and std::is_move_constructible_v<T>;

class cls {};

template <typename T> struct is_storage;

template <typename T, typename = void>
struct is_storage_impl : std::false_type {};

template <typename T>
struct is_storage_impl<
    T, std::void_t<decltype(std::declval<T>().data()),
                   decltype(std::declval<const T>().data())>> {
  static constexpr bool has_data =
      std::is_same_v<void *, decltype(std::declval<T>().data())>;
  static constexpr bool has_const_data =
      std::is_same_v<const void *, decltype(std::declval<const T>().data())>;
  static constexpr bool value = has_data && has_const_data and is_moveable_v<T>;
};

template <typename T> struct is_storage : public is_storage_impl<T, void> {};
#if __cplusplus > 201703L
// include if c++ std > c++17
template <typename T>
concept Storage = requires(T t, T other, const T ct, cls cl) {
  { t = std::move(other) } -> std::same_as<T &>;
  { T() };
  { t.data() } -> std::convertible_to<void *>;
  { ct.data() } -> std::convertible_to<const void *>;
  { t.template emplace<cls>(cl) };
};
template <typename T> inline constexpr bool is_storage_v = Storage<T>;
#else
template <typename T> inline constexpr bool is_storage_v = is_storage<T>::value;
#endif

/// simply utility "identity" type
template <typename T> struct Id {
  using type = T;
};

// signature utils

template <typename Sig> struct func_return_type;
template <typename Sig> struct func_first_arg;
template <typename Sig> struct func_is_const;
template <typename Sig> struct func_is_noexcept;

template <typename Ret, typename... Args>
struct func_return_type<Ret(Args...)> {
  using type = Ret;
};
template <typename Ret, typename... Args>
struct func_return_type<Ret(Args...) const> {
  using type = Ret;
};
template <typename Ret, typename... Args>
struct func_return_type<Ret(Args...) noexcept> {
  using type = Ret;
};
template <typename Ret, typename... Args>
struct func_return_type<Ret(Args...) const noexcept> {
  using type = Ret;
};

template <typename Ret, typename... Args> struct func_first_arg<Ret(Args...)> {
  using type = at_t<type_list<Args...>, 0>;
};
template <typename Ret, typename... Args>
struct func_first_arg<Ret(Args...) const> {
  using type = at_t<type_list<Args...>, 0>;
};
template <typename Ret, typename... Args>
struct func_first_arg<Ret(Args...) noexcept> {
  using type = at_t<type_list<Args...>, 0>;
};
template <typename Ret, typename... Args>
struct func_first_arg<Ret(Args...) const noexcept> {
  using type = at_t<type_list<Args...>, 0>;
};
template <typename Ret> struct func_first_arg<Ret() noexcept> {
  using type = void;
};
template <typename Ret> struct func_first_arg<Ret() const noexcept> {
  using type = void;
};

template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...)> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) const> : std::true_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) noexcept> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) const noexcept> : std::true_type {};

template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...)> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) const> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) noexcept> : std::true_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) const noexcept> : std::true_type {};

/// get the value type of a PropertySpec
template <typename PropertySpec>
using value_type_t = typename func_first_arg<PropertySpec>::type;

/// check if a PropertySpec is const
template <typename PropertySpec>
inline constexpr bool is_const_v =
    std::is_const_v<typename func_return_type<PropertySpec>::type>;

/// get the return type of a MethodSpec
template <typename MethodSpec>
using return_type_t = typename func_return_type<MethodSpec>::type;

template <typename T> struct is_method_spec : std::false_type {};
template <typename R, typename M, typename... Args>
struct is_method_spec<R(M, Args...)> : std::true_type {};
template <typename R, typename M, typename... Args>
struct is_method_spec<R(M, Args...) const> : std::true_type {};
template <typename R, typename M, typename... Args>
struct is_method_spec<R(M, Args...) noexcept> : std::true_type {};
template <typename R, typename M, typename... Args>
struct is_method_spec<R(M, Args...) const noexcept> : std::true_type {};

template <typename T>
inline constexpr bool is_method_spec_v = is_method_spec<T>::value;

template <typename T> struct is_property_spec : std::false_type {};
template <typename Name, typename Type>
struct is_property_spec<Name(Type)> : std::true_type {};
template <typename Name, typename Type>
struct is_property_spec<const Name(Type)> : std::true_type {};
template <typename Name, typename Type>
struct is_property_spec<Name(Type) noexcept> : std::true_type {};
template <typename Name, typename Type>
struct is_property_spec<const Name(Type) noexcept> : std::true_type {};

template <typename T>
inline constexpr bool is_property_spec_v = is_property_spec<T>::value;
} // namespace traits

/// check if T is a valid PropertySpec
template <typename T>
inline constexpr bool is_property_spec_v = traits::is_property_spec_v<T>;
/// get the value type of a PropertySpec
template <typename PropertySpec>
using value_type_t = traits::value_type_t<PropertySpec>;
/// get the name of a PropertySpec
template <typename PropertySpec>
using property_name_t =
    std::remove_const_t<typename traits::func_return_type<PropertySpec>::type>;
/// check if a PropertySpec is specified noexcept
template <typename PropertySpec>
inline constexpr bool is_nothrow_property_v =
    traits::func_is_noexcept<PropertySpec>::value;
/// check if a PropertySpec is const
template <typename PropertySpec>
inline constexpr bool is_const_property_v = traits::is_const_v<PropertySpec>;

/// check if T is a valid MethodSpec
template <typename T>
inline constexpr bool is_method_spec_v = traits::is_method_spec_v<T>;
/// get the return type of a MethodSpec
template <typename MethodSpec>
using return_type_t = traits::return_type_t<MethodSpec>;
/// get the name of a MethodSpec
template <typename MethodSpec>
using method_name_t = typename traits::func_first_arg<MethodSpec>::type;
/// evaluates to true if MethodSpec if specified noexcept
template <typename MethodSpec>
inline constexpr bool is_nothrow_method_v =
    traits::func_is_noexcept<MethodSpec>::value;
/// evaluates to true if MethodSpec if specified const
template <typename MethodSpec>
inline constexpr bool is_const_method_v =
    traits::func_is_const<MethodSpec>::value;
/// evaluates to true if T satisfies the storage concept
template <typename T>
inline constexpr bool is_storage_v = traits::is_storage_v<T>;
} // namespace poly
#endif
