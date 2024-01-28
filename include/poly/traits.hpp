/**
 * @file poly/traits.hpp
 */
#ifndef POLY_TRAITS_HPP
#define POLY_TRAITS_HPP

#include "poly/type_list.hpp"

#include <type_traits>
#include <utility>

namespace poly {

namespace traits {

template <typename T>
inline constexpr bool is_moveable_v =
    std::is_move_assignable_v<T> and std::is_move_constructible_v<T>;

class cls {
public:
  template <typename... A> cls(A... a) : data_{1} {
    ((void)a, ...);
    (void)data_;
  }

private:
  char data_{0};
};

template <typename T> struct is_storage;

template <typename T, typename = void>
struct is_storage_impl : std::false_type {};

template <typename T, typename = void>
struct has_single_lvalue_ref_emplace : std::false_type {};
template <typename T>
struct has_single_lvalue_ref_emplace<
    T,
    std::void_t<decltype(T{}.template emplace<cls>(std::declval<cls &>()))>> {
  static constexpr bool value =
      std::is_same_v<cls &, decltype(T{}.template emplace<cls>(
                                std::declval<cls &>()))>;
};
template <typename T, typename = void>
struct has_mutli_value_emplace : std::false_type {};
template <typename T>
struct has_mutli_value_emplace<
    T, std::void_t<decltype(T{}.template emplace<cls>(1, 2, 3, 4))>> {
  static constexpr bool value =
      std::is_same_v<cls &, decltype(T{}.template emplace<cls>(1, 2, 3, 4))>;
};
template <typename T>
struct is_storage_impl<
    T,
    std::void_t<decltype(std::declval<T>() = std::move(std::declval<T &&>())),
                decltype(std::declval<T>().data()),
                decltype(std::declval<const T>().data())>> {

  static constexpr bool has_data =
      std::is_same_v<void *, decltype(std::declval<T>().data())>;
  static constexpr bool has_const_data =
      std::is_same_v<const void *, decltype(std::declval<const T>().data())>;
  static constexpr bool value =
      has_data && has_const_data && is_moveable_v<T> &&
      (has_single_lvalue_ref_emplace<T, void>::value or
       has_mutli_value_emplace<T, void>::value);
};

template <typename T>
struct is_storage : public std::conditional_t<is_storage_impl<T, void>::value,
                                              std::true_type, std::false_type> {
};

template <typename T> struct is_type_list : std::false_type {};

template <template <typename...> typename L, typename... Ts>
struct is_type_list<L<Ts...>> : std::true_type {};

template <typename T> inline constexpr bool is_storage_v = is_storage<T>::value;

/// simply utility "identity" type
template <typename T> struct Id {
  using type = T;
};

/// @{

/// gets a function types return type.
/// @{
template <typename Sig> struct func_return_type;
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
/// @}

/// gets a function types first argument, or void if the function has no
/// arguments.
/// @{
template <typename Sig> struct func_first_arg;
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
template <typename Ret> struct func_first_arg<Ret()> {
  using type = void;
};
template <typename Ret> struct func_first_arg<Ret() const> {
  using type = void;
};
/// @}

/// evaluates to true if the function type is const
/// @{
template <typename Sig> struct func_is_const;
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...)> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) const> : std::true_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) noexcept> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_const<Ret(Args...) const noexcept> : std::true_type {};
/// @}

/// evaluates to true if the function type is noexcept
/// @{
template <typename Sig> struct func_is_noexcept;
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...)> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) const> : std::false_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) noexcept> : std::true_type {};
template <typename Ret, typename... Args>
struct func_is_noexcept<Ret(Args...) const noexcept> : std::true_type {};
/// @}

/// evaluates to true if the function type is noexcept
/// @{
template <typename Sig> struct func_args;
template <typename Ret, typename... Args> struct func_args<Ret(Args...)> {
  using type = type_list<Args...>;
};
template <typename Ret, typename... Args> struct func_args<Ret(Args...) const> {
  using type = type_list<Args...>;
};
template <typename Ret, typename... Args>
struct func_args<Ret(Args...) noexcept> {
  using type = type_list<Args...>;
};
template <typename Ret, typename... Args>
struct func_args<Ret(Args...) const noexcept> {
  using type = type_list<Args...>;
};
/// @}

/// evaluates to true if F is incovable with the signature Sig
/// @{
template <typename Sig, typename F> struct invocable_r;
template <typename Ret, typename... Args, typename F>
struct invocable_r<Ret(Args...), F> : std::is_invocable_r<Ret, F, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable_r<Ret(Args...) const, F>
    : std::is_invocable_r<Ret, std::add_const_t<F>, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable_r<Ret(Args...) noexcept, F>
    : std::is_nothrow_invocable_r<Ret, F, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable_r<Ret(Args...) const noexcept, F>
    : std::is_nothrow_invocable_r<Ret, std::add_const_t<F>, Args...> {};

template <typename Sig, typename F> struct invocable;
template <typename Ret, typename... Args, typename F>
struct invocable<Ret(Args...), F> : std::is_invocable<F, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable<Ret(Args...) const, F>
    : std::is_invocable<std::add_const_t<F>, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable<Ret(Args...) noexcept, F>
    : std::is_nothrow_invocable<F, Args...> {};
template <typename Ret, typename... Args, typename F>
struct invocable<Ret(Args...) const noexcept, F>
    : std::is_nothrow_invocable<std::add_const_t<F>, Args...> {};
/// @}
/// @}

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

template <auto value>
using smallest_uint_to_contain =
    std::conditional_t <
    std::size_t(value)<
        std::size_t(256), std::uint8_t,
        std::conditional_t<std::size_t(value) < (std::size_t(1) << 16), std::uint16_t,
                           std::conditional_t<std::size_t(value) < (std::size_t(1) << 32),
                                              std::uint32_t, std::uint64_t>>>;
} // namespace traits

/// @addtogroup traits type traits
/// @{

/// @addtogroup PropertySpec PropertySpec Concept
/// A PropertySpec defines the name of a property and its value type. It is a
/// function type with the following allowed patterns:
/// - 'PropertyName(ValueType)': a read/write property with name PropertyName
/// and value type ValueType.
/// - 'const PropertyName(ValueType)': a read only property
/// - 'PropertyName(ValueType) noexcept': a read/write property with noexcept
/// access
/// - 'const PropertyName(ValueType) noexcept': a read only property with
/// noexcept access
/// @{

/// evaluates to true if T is a valid PropertySpec.
template <typename T>
inline constexpr bool is_property_spec_v = traits::is_property_spec_v<T>;

/// get the value type of a PropertySpec
template <typename PropertySpec>
using value_type_t = traits::value_type_t<PropertySpec>;

/// get the name of a PropertySpec
template <typename PropertySpec>
using property_name_t =
    std::remove_const_t<typename traits::func_return_type<PropertySpec>::type>;

template <typename T, typename PropertySpec> std::false_type has_validator(...);
template <typename T, typename PropertySpec>
std::integral_constant<
    bool, std::is_same_v<
              decltype(check(
                  property_name_t<PropertySpec>{}, std::declval<const T &>(),
                  std::declval<const value_type_t<PropertySpec> &>())),
              bool>>
has_validator(decltype(sizeof(
    check(property_name_t<PropertySpec>{}, std::declval<const T &>(),
          std::declval<const value_type_t<PropertySpec> &>()))));

/// evaluates to true if a PropertySpec is specified noexcept
template <typename PropertySpec>
inline constexpr bool is_nothrow_property_v =
    traits::func_is_noexcept<PropertySpec>::value;

template <typename T, typename PropertySpec>
inline constexpr bool has_validator_v =
    decltype(has_validator<T, PropertySpec>(0))::value;

/// evaluates to true if a PropertySpec is const, i.e. read only.
template <typename PropertySpec>
inline constexpr bool is_const_property_v = traits::is_const_v<PropertySpec>;

/// @}

/// @addtogroup MethodSpec MethodSpec Concept
///
/// A @ref MethodSpec defines the return type, name, and arguments of a method.
///
/// A method, in the context of poly, is a member function to be called on an
/// interface. It is a function signature type with the allowed patterns:
/// - ReturnType(MethodName, ArgumentTypes...): specifies a method with return
/// type ReturnType, name MethodName, and arguments of type ArgumentTypes.
/// - ReturnType(MethodName, ArgumentTypes...) const: specifies a const method,
/// i.e. the access to the object it is called on is immutable.
/// - ReturnType(MethodName, ArgumentTypes...) noexcept: specifies a non
/// throwing method.
/// - ReturnType(MethodName, ArgumentTypes...) const noexcept: specifies a const
/// non throwing method.
///
/// The MethodName is a tag type and does not contain any data. It is simply
/// used to tag @ref method_extension "extend" to enable methods with identical
/// arguments and overloaded methods.
/// @{

/// evaluates to true if T is a valid MethodSpec.
template <typename T>
inline constexpr bool is_method_spec_v = traits::is_method_spec_v<T>;

/// get the return type of a MethodSpec.
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

/// @}

/// @addtogroup Storage Storage Concept
///
/// Storage is a type erased container. Objects of arbitrary types can be
/// emplaced, that is they are stored owning or non-owning, in a Storage object.
///
/// Objects are emplaced into a Storage with its emplace() template method. The
/// first template parameter specifies the type of the emplaced object. The
/// rest, if there are any, must be deductible by the compiler. emplace()
/// returns a reference to the emplaced object. Non-owning Storages take exactly
/// one (possibly const in case of non modifiable storage) lvalue reference
/// parameter of the type to be emplaced. Owning storages take an arbitrary
/// amount of arguments used for constructing the type to be emplaced.
///
/// Upon emplacement, the stored object can be accessed as a (const) void
/// pointer through the Storages data() method. Calling data() on an empty
/// Storage returns nullptr.
///
/// Emplacing a new object into a Storage already containing an object will
/// destroy the currently emplaced object and emplace the new object.
///
/// Destroying a Storage through its destructor will destroy the emplaced object
/// appropriately, i.e. call the destructor of the emplaced object in case of
/// an owning Storage, or simply reset its internal reference/pointer in the
/// case of non-owning Storage.
///
/// In addition, a Storage is at least moveable, i.e. move constructible
/// and move assignable, as well as default constructible. A default constructed
/// Storage is empty. Moved from Storages are empty and destroy their contained
/// object before the move operation returns, i.e. before the move
/// constructor/assignment operator returns to the caller.
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
template <typename T>
inline constexpr bool is_storage_v = traits::is_storage_v<T>;
/// @}

/// evaluates to true if F is invocable with the signature Sig.
template <typename Sig, typename F>
inline constexpr bool is_invocable_v = traits::invocable_r<Sig, F>::value;
/// @}

#if __cplusplus > 201703L
// enabled if c++ std > c++17
/** @concept Storage
 * @see poly::is_storage_v
 */
template <typename T>
concept Storage = ::poly::is_storage_v<T>;

/** @concept MethodSpecification
 * @see poly::is_method_spec_v
 */
template <typename T>
concept MethodSpecification = ::poly::is_method_spec_v<T>;

template <typename T>
concept PropertySpecification = ::poly::is_property_spec_v<T>;

#endif
} // namespace poly

#if __cplusplus > 201703L
// enabled if c++ std > c++17
#define POLY_STORAGE ::poly::Storage
#define POLY_METHOD_SPEC ::poly::MethodSpecification
#define POLY_PROP_SPEC ::poly::PropertySpecification
#else
#define POLY_STORAGE typename
#define POLY_METHOD_SPEC typename
#define POLY_PROP_SPEC typename
#endif

#endif
