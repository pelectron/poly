#ifndef POLY_TYPE_LIST_HPP
#define POLY_TYPE_LIST_HPP
#include "poly/always_false.hpp"
#include <cstddef>
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
template <typename List, typename T, std::size_t N> struct index_of_impl;
template <typename... Ts, typename T1, typename T, std::size_t N>
struct index_of_impl<type_list<T1, Ts...>, T, N>
    : index_of_impl<type_list<Ts...>, T, N + 1> {};
template <typename... Ts, typename T, std::size_t N>
struct index_of_impl<type_list<T, Ts...>, T, N> {
  static constexpr std::size_t value = N;
};
template <typename T, std::size_t N> struct index_of_impl<type_list<>, T, N> {
  static_assert(detail::always_false<T>, "T not found in List");
};
template <typename... Ts, typename T>
struct index_of<type_list<Ts...>, T> : index_of_impl<type_list<Ts...>, T, 0> {};

template <typename List, std::size_t N, std::size_t I> struct at_impl;

template <typename... Ts, typename T1, std::size_t N, std::size_t I>
struct at_impl<type_list<T1, Ts...>, N, I>
    : at_impl<type_list<Ts...>, N + 1, I> {};

template <typename... Ts, typename T1, std::size_t I>
struct at_impl<type_list<T1, Ts...>, I, I> {
  using type = T1;
};

template <typename List, std::size_t I> struct at : at_impl<List, 0, I> {};

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

template <typename List, template <typename> typename F> struct transform;

template <typename... Ts, template <typename> typename F>
struct transform<type_list<Ts...>, F> {
  using type = type_list<typename F<Ts>::type...>;
};
template <typename List1, typename List2> struct concat;
template <typename... Ts, typename... Us>
struct concat<type_list<Ts...>, type_list<Us...>> {
  using type = type_list<Ts..., Us...>;
};
template <typename List> struct contains_duplicates;

template <typename List, typename T> struct contains_duplicates_impl;

template <typename T, typename T1, typename... Ts>
struct contains_duplicates_impl<type_list<T1, Ts...>, T> {
  static constexpr bool value =
      std::is_same_v<T, T1> or (std::is_same_v<T, Ts> or ...) or
      contains_duplicates_impl<type_list<Ts...>, T1>::value;
};
template <typename T>
struct contains_duplicates_impl<type_list<>, T> : std::false_type {};

template <typename T, typename... Ts>
struct contains_duplicates<type_list<T, Ts...>>
    : contains_duplicates_impl<type_list<Ts...>, T> {};
template <> struct contains_duplicates<type_list<>> : std::false_type {};

static_assert(not contains_duplicates<type_list<int>>::value);
static_assert(not contains_duplicates<type_list<int, char>>::value);
static_assert(not contains_duplicates<type_list<int, char, double>>::value);
static_assert(contains_duplicates<type_list<int, char, int>>::value);
static_assert(contains_duplicates<type_list<int, int, char>>::value);

template <typename ListOut, typename ListIn> struct remove_duplicates_impl;
template <typename... Ts>
struct remove_duplicates_impl<type_list<Ts...>, type_list<>> {
  using type = type_list<Ts...>;
};
template <typename... Us, typename T, typename... Ts>
struct remove_duplicates_impl<type_list<Us...>, type_list<T, Ts...>> {
  using new_list = std::conditional_t<(std::is_same_v<T, Us> or ...),
                                      type_list<Us...>, type_list<Us..., T>>;
  using type =
      typename remove_duplicates_impl<new_list, type_list<Ts...>>::type;
};

template <typename List>
struct remove_duplicates : remove_duplicates_impl<type_list<>, List> {};

static_assert(
    std::is_same_v<typename remove_duplicates<type_list<int, char, int>>::type,
                   type_list<int, char>>);
static_assert(
    std::is_same_v<typename remove_duplicates<type_list<int, int, int>>::type,
                   type_list<int>>);
static_assert(std::is_same_v<
              typename remove_duplicates<type_list<int, char, double>>::type,
              type_list<int, char, double>>);

template <typename List> struct list_size;
template <typename... Ts> struct list_size<type_list<Ts...>> {
  static constexpr size_t value = sizeof...(Ts);
};

template <typename List> struct conjunction;
template <typename... Ts> struct conjunction<type_list<Ts...>> {
  static constexpr bool value = (Ts::value && ...);
};

template <typename T> struct is_type_list : std::false_type {};
template <typename... Ts>
struct is_type_list<type_list<Ts...>> : std::true_type {};

} // namespace detail

/// push T to the front of the type_list List
template <typename List, typename T>
using push_front_t = typename detail::push_front<List, T>::type;

/// get the position of T in the type_list List
template <typename List, typename T>
inline constexpr std::size_t index_of_v = detail::index_of<List, T>::value;

/// get the I-th type in the type_list List
template <typename List, std::size_t I>
using at_t = typename detail::at<List, I>::type;

/// filter list by predicate
template <typename List, template <typename> typename Predicate>
using filter_t = typename detail::filter<List, Predicate>::type;

template <typename List, template <typename> typename F>
using transform_t = typename detail::transform<List, F>::type;

template <typename List>
inline constexpr bool contains_duplicates_v =
    detail::contains_duplicates<List>::value;

template <typename List>
inline constexpr bool conjunction_v = detail::conjunction<List>::value;

template <typename T>
inline constexpr bool is_type_list_v = detail::is_type_list<T>::value;
} // namespace poly
#endif
