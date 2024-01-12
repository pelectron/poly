#ifndef POLY_METHOD_HPP
#define POLY_METHOD_HPP

#define POLY_METHOD(method)                                                    \
  struct method;                                                               \
  struct method {                                                              \
    template <typename Self, typename MethodSpec> struct injector;               \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(method, Args...)> {                                \
      Ret method(Args... args) {                                               \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct method>(                             \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(method, Args...) const> {                          \
      Ret method(Args... args) const {                                         \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct method>(                             \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(method, Args...) noexcept> {                       \
      Ret method(Args... args) noexcept {                                      \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct method>(                             \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(method, Args...) const noexcept> {                 \
      Ret method(Args... args) const noexcept {                                \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct method>(                             \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
  };                                                                           \
                                                                               \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(method, T &t, Args &&...args) noexcept(                \
      std::declval<T>().method(                                                \
          std::forward<Args>(std::declval<decltype(args)>())...)) {            \
    return t.method(std::forward<Args>(args)...);                              \
  }                                                                            \
                                                                               \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(method, const T &t, Args &&...args) noexcept(          \
      std::declval<const T>().method(                                          \
          std::forward<Args>(std::declval<decltype(args)>())...)) {            \
    return t.method(std::forward<Args>(args)...);                              \
  }

namespace poly {

/// extension point for the MethodSpec 'Ret(Method,Args...)[noexcept]'.
template <typename Ret, typename Method, typename T, typename... Args>
Ret extend(Method, T &t, Args &&...args);

/// extension point for the MethodSpec 'Ret(Method,Args...)const [noexcept]'.
template <typename Ret, typename Method, typename T, typename... Args>
Ret extend(Method, const T &t, Args &&...args);
} // namespace poly
#endif
