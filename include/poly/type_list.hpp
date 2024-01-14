#ifndef POLY_TYPE_LIST_HPP
#define POLY_TYPE_LIST_HPP
#include "poly/always_false.hpp"
#include <type_traits>

namespace poly {

/// type list
template <typename...> struct type_list {};

namespace detail {
template <typename List, typename T> struct push_front;
template <typename... Ts, typename T> struct push_front<type_list<Ts...>, T> {
  using type = type_list<T, Ts...>;
};
template <typename List, typename T> struct index_of;
template <typename List, typename T, size_t N> struct index_of_impl;
template <typename... Ts, typename T1, typename T, size_t N>
struct index_of_impl<type_list<T1, Ts...>, T, N>
    : index_of_impl<type_list<Ts...>, T, N + 1> {};
template <typename... Ts, typename T, size_t N>
struct index_of_impl<type_list<T, Ts...>, T, N> {
  static constexpr size_t value = N;
};
template <typename T, size_t N> struct index_of_impl<type_list<>, T, N> {
  static_assert(detail::always_false<T>, "T not found in List");
};
template <typename... Ts, typename T>
struct index_of<type_list<Ts...>, T> : index_of_impl<type_list<Ts...>, T, 0> {};

template <typename List, size_t N, size_t I> struct at_impl;

template <typename... Ts, typename T1, size_t N, size_t I>
struct at_impl<type_list<T1, Ts...>, N, I>
    : at_impl<type_list<Ts...>, N + 1, I> {};

template <typename... Ts, typename T1, size_t I>
struct at_impl<type_list<T1, Ts...>, I, I> {
  using type = T1;
};

template <typename List, size_t I> struct at : at_impl<List, 0, I> {};

template <typename InList, typename OutList, template <typename> typename Trait>
struct filter_impl;

template <typename T, typename... Us, template <typename> typename Trait>
struct filter_impl<type_list<T>, type_list<Us...>, Trait> {
  using type = std::conditional_t<Trait<T>::value, type_list<Us..., T>,
                                  type_list<Us...>>;
};

template <typename T, typename... Ts, typename... Us,
          template <typename> typename Trait>
struct filter_impl<type_list<T, Ts...>, type_list<Us...>, Trait> {
  using list = std::conditional_t<Trait<T>::value, type_list<Us..., T>,
                                  type_list<Us...>>;
  using type = typename filter_impl<type_list<Ts...>, list, Trait>::type;
};

template <typename List, template <typename> typename Trait>
struct filter : filter_impl<List, type_list<>, Trait> {};

} // namespace detail

/// push T to the front of the type_list List
template <typename List, typename T>
using push_front_t = typename detail::push_front<List, T>::type;

/// get the position of T in the type_list List
template <typename List, typename T>
inline constexpr size_t index_of_v = detail::index_of<List, T>::value;

/// get the I-th type in the type_list List
template <typename List, size_t I>
using at_t = typename detail::at<List, I>::type;

/// filter list by predicate
template <typename List, template <typename> typename Predicate>
using filter_t = typename detail::filter<List, Predicate>::type;
} // namespace poly
#endif
