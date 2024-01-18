#ifndef POLY_METHOD_HPP
#define POLY_METHOD_HPP
#include "poly/config.hpp"

#include <type_traits>

#if POLY_USE_MACROS == 1
#define POLY_METHOD(MethodName)                                                \
  POLY_METHOD_IMPL(MethodName)                                                 \
  POLY_DEFAULT_EXTEND_IMPL(MethodName)
#if (POLY_USE_INJECTOR == 1) || (POLY_USE_METHOD_INJECTOR == 1)
#define POLY_METHOD_IMPL(MethodName)                                           \
  struct MethodName {                                                          \
    template <typename Self, typename MethodSpec> struct injector;             \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...)> {                          \
      Ret MethodName(Args... args) {                                           \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) const> {                    \
      Ret MethodName(Args... args) const {                                     \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) noexcept> {                 \
      Ret MethodName(Args... args) noexcept {                                  \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) const noexcept> {           \
      Ret MethodName(Args... args) const noexcept {                            \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
  };

#else
#define POLY_METHOD_IMPL(MethodName)                                           \
  struct MethodName {};
#endif

#if POLY_USE_DEFAULT_EXTEND == 1
#define POLY_DEFAULT_EXTEND_IMPL(MethodName)                                   \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(MethodName, T &t, Args &&...args) noexcept(            \
      noexcept(std::declval<T>().MethodName(                                   \
          std::forward<Args>(std::declval<decltype(args)>())...))) {           \
    return t.MethodName(std::forward<Args>(args)...);                          \
  }                                                                            \
                                                                               \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(MethodName, const T &t, Args &&...args) noexcept(      \
      noexcept(std::declval<const T>().MethodName(                             \
          std::forward<Args>(std::declval<decltype(args)>())...))) {           \
    return t.MethodName(std::forward<Args>(args)...);                          \
  }
#else
#define POLY_DEFAULT_EXTEND_IMPL(MethodName)
#endif

#define POLY_USE_OVERLOAD(MethodName, Signature)                               \
  using poly::detail::MethodInjector<                                          \
      poly::detail::insert_method_name_t<struct MethodName, Signature>,        \
      self_type>::MethodName;

#endif

namespace poly {

/// extension point for the MethodSpec 'Ret(Method,Args...)[noexcept]'.
template <typename Ret, typename MethodName, typename T, typename... Args>
Ret extend(MethodName, T &t, Args &&...args);

/// extension point for the MethodSpec 'Ret(Method,Args...)const [noexcept]'.
template <typename Ret, typename MethodName, typename T, typename... Args>
Ret extend(MethodName, const T &t, Args &&...args);

namespace detail {
template <typename M, typename Sig> struct insert_method_name;

template <typename M, typename Ret, typename... Args>
struct insert_method_name<M, Ret(Args...)> {
  using type = Ret(M, Args...);
};
template <typename M, typename Ret, typename... Args>
struct insert_method_name<M, Ret(Args...) const> {
  using type = Ret(M, Args...) const;
};
template <typename M, typename Ret, typename... Args>
struct insert_method_name<M, Ret(Args...) noexcept> {
  using type = Ret(M, Args...) noexcept;
};
template <typename M, typename Ret, typename... Args>
struct insert_method_name<M, Ret(Args...) const noexcept> {
  using type = Ret(M, Args...) const noexcept;
};
template <typename M, typename Sig>
using insert_method_name_t = typename insert_method_name<M, Sig>::type;

/// default method injector does nothing
template <typename MethodSpec, typename Self, typename = void>
struct MethodInjector {};

/// if the method was created with the POLY_METHOD macro, there will be an
/// innner template named "injector" present, which is templated on a type Self
/// and the MEthodSpec. The injector will link the Method with its real name,
/// i.e. "Method". This allows the end user to use the syntax
/// obj.Method(args...) instead of obj.call<Method>(args...).
template <typename R, typename... A, typename M, typename Self>
struct MethodInjector<
    R(M, A...), Self,
    std::void_t<typename M::template injector<Self, R(M, A...)>>>
    : M::template injector<Self, R(M, A...)> {};

template <typename R, typename... A, typename M, typename Self>
struct MethodInjector<
    R(M, A...) const, Self,
    std::void_t<typename M::template injector<const Self, R(M, A...) const>>>
    : M::template injector<Self, R(M, A...) const> {};

template <typename R, typename... A, typename M, typename Self>
struct MethodInjector<
    R(M, A...) noexcept, Self,
    std::void_t<typename M::template injector<Self, R(M, A...) noexcept>>>
    : M::template injector<Self, R(M, A...) noexcept> {};

template <typename R, typename... A, typename M, typename Self>
struct MethodInjector<
    R(M, A...) const noexcept, Self,
    std::void_t<typename M::template injector<Self, R(M, A...) const noexcept>>>
    : M::template injector<Self, R(M, A...) const noexcept> {};

} // namespace detail
} // namespace poly
#endif
