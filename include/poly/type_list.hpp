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
#ifndef POLY_TYPE_LIST_HPP
#define POLY_TYPE_LIST_HPP
#include "poly/always_false.hpp"

#include <cstddef>
#include <type_traits>

namespace poly {

/// type list
template<typename...>
struct type_list {};

namespace detail {
  template<typename List, typename T>
  struct push_front;
  template<template<typename...> typename List, typename... Ts, typename T>
  struct push_front<List<Ts...>, T> {
    using type = List<T, Ts...>;
  };

  template<typename List, template<typename...> typename T>
  struct apply;
  template<template<typename...> typename List,
           template<typename...> typename T, typename... Ts>
  struct apply<List<Ts...>, T> {
    using type = T<Ts...>;
  };

  /// Gets the index of T in the type list, if T is in the List.
  /// Else a static assertion is triggered.
  /// @{

  template<typename List, typename T, std::size_t N>
  struct index_of_impl;
  template<template<typename...> typename List, typename... Ts, typename T1,
           typename T, std::size_t N>
  struct index_of_impl<List<T1, Ts...>, T, N>
      : index_of_impl<List<Ts...>, T, N + 1> {};
  template<template<typename...> typename List, typename... Ts, typename T,
           std::size_t N>
  struct index_of_impl<List<T, Ts...>, T, N> {
    static constexpr std::size_t value = N;
  };
  template<template<typename...> typename List, typename T, std::size_t N>
  struct index_of_impl<List<>, T, N> {
    static_assert(detail::always_false<T>, "T not found in List");
  };

  template<typename List, typename T>
  struct index_of;

  template<template<typename...> typename List, typename... Ts, typename T>
  struct index_of<List<Ts...>, T> : index_of_impl<List<Ts...>, T, 0> {};
  /// @}

  template<typename List, std::size_t N, std::size_t I>
  struct at_impl {
    using type = List;
  };

  template<template<typename...> typename List, typename... Ts, typename T1,
           std::size_t N, std::size_t I>
  struct at_impl<List<T1, Ts...>, N, I> {
    using type =
        std::conditional_t<N == I, T1,
                           typename at_impl<List<Ts...>, N + 1, I>::type>;
  };

  template<typename List, std::size_t I>
  struct at {
    using type = typename at_impl<List, 0, I>::type;
  };

  static_assert(
      std::is_same_v<char, typename at<type_list<int, char, float>, 1>::type>);
  static_assert(
      std::is_same_v<float, typename at<type_list<int, char, float>, 2>::type>);

  template<typename InList, typename OutList, template<typename> typename Trait>
  struct filter_impl;

  template<template<typename...> typename List, typename T, typename... Us,
           template<typename> typename Trait>
  struct filter_impl<List<T>, List<Us...>, Trait> {
    using type =
        std::conditional_t<Trait<T>::value, List<Us..., T>, List<Us...>>;
  };

  template<template<typename...> typename List, typename T, typename... Ts,
           typename... Us, template<typename> typename Trait>
  struct filter_impl<List<T, Ts...>, List<Us...>, Trait> {
    using list =
        std::conditional_t<Trait<T>::value, List<Us..., T>, List<Us...>>;
    using type = typename filter_impl<List<Ts...>, list, Trait>::type;
  };

  template<typename List, template<typename> typename Trait>
  struct filter;
  template<template<typename...> typename List,
           template<typename> typename Trait, typename... Ts>
  struct filter<List<Ts...>, Trait> : filter_impl<List<Ts...>, List<>, Trait> {
  };

  /// transform each element in List by applying F on each elment in the List
  ///
  /// Example:
  /// using L1 = type_list<char,const int,const double>;
  /// using transformed_list = typename transform<L1,std::remove_const>::type;
  /// static_assert(std::is_same_v<transformed_list,type_list<char,int,double>>);
  /// @tparam List a TypeList
  /// @tparam F a unary meta function which must provied an inner type alias
  /// called type.
  /// @{
  template<typename List, template<typename> typename F>
  struct transform;

  template<template<typename...> typename List, typename... Ts,
           template<typename> typename F>
  struct transform<List<Ts...>, F> {
    using type = List<typename F<Ts>::type...>;
  };
  /// @}

  /// concatenate two TypeLists
  /// Example:
  /// using L1 = type_list<char,int,double>;
  /// using L2 = type_list<unsigned, float>;
  /// using cat = typename concat<L1,L2>::type;
  /// static_assert(std::is_same_v<cat,type_list<char,int,double,unsigned,float>>);
  /// @{
  template<typename List1, typename List2>
  struct concat;
  template<template<typename...> typename List, typename... Ts, typename... Us>
  struct concat<List<Ts...>, List<Us...>> {
    using type = List<Ts..., Us...>;
  };
  /// @}

  /// check if a TypeList contains a T
  template<typename List, typename T>
  struct contains;

  template<template<typename...> typename List, typename... Ts, typename T>
  struct contains<List<Ts...>, T> {
    static constexpr bool value = (std::is_same_v<T, Ts> or ...);
  };

  /// check if a TypeList contains duplicate entries
  /// @{
  template<typename List>
  struct contains_duplicates;

  template<typename List, typename T>
  struct contains_duplicates_impl;

  template<template<typename...> typename List, typename T, typename T1,
           typename... Ts>
  struct contains_duplicates_impl<List<T1, Ts...>, T> {
    static constexpr bool value =
        std::is_same_v<T, T1> or (std::is_same_v<T, Ts> or ...) or
        contains_duplicates_impl<List<Ts...>, T1>::value;
  };
  template<template<typename...> typename List, typename T>
  struct contains_duplicates_impl<List<>, T> : std::false_type {};

  template<template<typename...> typename List, typename T, typename... Ts>
  struct contains_duplicates<List<T, Ts...>>
      : contains_duplicates_impl<List<Ts...>, T> {};
  template<template<typename...> typename List>
  struct contains_duplicates<List<>> : std::false_type {};
  /// @}

  static_assert(not contains_duplicates<type_list<int>>::value);
  static_assert(not contains_duplicates<type_list<int, char>>::value);
  static_assert(not contains_duplicates<type_list<int, char, double>>::value);
  static_assert(contains_duplicates<type_list<int, char, int>>::value);
  static_assert(contains_duplicates<type_list<int, int, char>>::value);

  /// remove duplicate etries in a TypeList.
  /// @{
  template<typename ListOut, typename ListIn>
  struct remove_duplicates_impl;
  template<template<typename...> typename List, typename... Ts>
  struct remove_duplicates_impl<List<Ts...>, List<>> {
    using type = List<Ts...>;
  };
  template<template<typename...> typename List, typename... Us, typename T,
           typename... Ts>
  struct remove_duplicates_impl<List<Us...>, List<T, Ts...>> {
    using new_list = std::conditional_t<(std::is_same_v<T, Us> or ...),
                                        List<Us...>, List<Us..., T>>;
    using type = typename remove_duplicates_impl<new_list, List<Ts...>>::type;
  };

  template<typename List>
  struct remove_duplicates;
  template<template<typename...> typename List, typename... Ts>
  struct remove_duplicates<List<Ts...>>
      : remove_duplicates_impl<List<>, List<Ts...>> {};
  /// @}

  static_assert(std::is_same_v<
                typename remove_duplicates<type_list<int, char, int>>::type,
                type_list<int, char>>);
  static_assert(
      std::is_same_v<typename remove_duplicates<type_list<int, int, int>>::type,
                     type_list<int>>);
  static_assert(std::is_same_v<
                typename remove_duplicates<type_list<int, char, double>>::type,
                type_list<int, char, double>>);

  template<typename List>
  struct list_size;
  template<template<typename...> typename List, typename... Ts>
  struct list_size<List<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
  };

  template<typename List>
  struct conjunction;
  template<template<typename...> typename List, typename... Ts>
  struct conjunction<List<Ts...>> {
    static constexpr bool value = (Ts::value && ...);
  };

  template<typename T>
  struct is_type_list : std::false_type {};
  template<template<typename...> typename List, typename... Ts>
  struct is_type_list<List<Ts...>> : std::true_type {};

} // namespace detail

/// push T to the front of the TypeList List
template<typename List, typename T>
using push_front_t = typename detail::push_front<List, T>::type;

/// get the position of T in List
template<typename List, typename T>
inline constexpr std::size_t index_of_v = detail::index_of<List, T>::value;

/// get the I-th type in List
template<typename List, std::size_t I>
using at_t = typename detail::at<List, I>::type;

/// filter List by Predicate. For every element E in List, if
/// Predicate<E>::value is true, E is added to the output, else E is
/// discarded.
///
/// Example:
/// using L1 = type_list<char,const int,const double>;
/// using filtered_list = filter_t<L1,std::is_const>;
/// static_assert(std::is_same_v<filtered_list,type_list<const int, const
/// double>>);
///
/// @tparam List a TypeList
/// @tparam Predicate a unary meta function which must provide a static
/// constexr member called value.
template<typename List, template<typename> typename Predicate>
using filter_t = typename detail::filter<List, Predicate>::type;

/// transform each element in List by applying F on each elment in the List
///
/// Example:
/// using L1 = type_list<char,const int,const double>;
/// using transformed_list = transform_t<L1,std::remove_const>;
/// static_assert(std::is_same_v<transformed_list,type_list<char,int,double>>);
/// @tparam List a TypeList
/// @tparam F a unary meta function which must provied an inner type alias
/// called type.
template<typename List, template<typename> typename F>
using transform_t = typename detail::transform<List, F>::type;

/// check if List contains duplicate elements
/// @tparam List a TypeList
template<typename List>
inline constexpr bool contains_duplicates_v =
    detail::contains_duplicates<List>::value;

/// performs (Es::value&&...) for the elements Es in the List
/// @tparam List a TypeList. Each element E in List must contain a static
/// constexpr booleanish member called value.
template<typename List>
inline constexpr bool conjunction_v = detail::conjunction<List>::value;

/// check if T is a TypeList, that is T refers to a specialization of a
/// template class U, where U takes a template type parameter pack and no
/// non-type template arguments. In short: only true if T == U<Ts...>, where
/// Ts... are the elements of the list.
///
/// Example:
/// static_assert(is_type_list_v<type_list<char>>);
/// static_assert(is_type_list_v<type_list<char, double>>);
/// static_assert(is_type_list_v<type_list<char, double>>);
/// static_assert(is_type_list_v<std::tuple<char, double>>);
/// static_assert(is_type_list_v<std::variant<char, double>>);
///
/// @tparam T type to check
template<typename T>
inline constexpr bool is_type_list_v = detail::is_type_list<T>::value;

template<typename List, typename T>
inline constexpr bool contains_v = detail::contains<List, T>::value;

template<typename List, template<typename...> typename T>
using apply_t = typename detail::apply<List, T>::type;

#if __cplusplus >= 202002L
/// TypeList concept
template<typename T>
concept TypeList = is_type_list_v<T>;
#endif

} // namespace poly

#endif
