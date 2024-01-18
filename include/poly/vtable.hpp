#ifndef POLY_VTABLE_HPP
#define POLY_VTABLE_HPP
#include "poly/config.hpp"

#include "poly/traits.hpp"
#include "poly/method.hpp"
#include <utility>
namespace poly {
namespace detail {

template <typename MethodSpec> struct trampoline;

template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...)> {
  template <typename T>
  static constexpr Ret jump(Method, void *t, Args... args) {
    using poly::extend;
    return extend(Method{}, *static_cast<T *>(t), std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) const> {
  template <typename T>
  static constexpr Ret jump(Method, const void *t, Args... args) {
    using poly::extend;
    return extend(Method{}, *static_cast<const T *>(t),
                  std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) noexcept> {
  template <typename T>
  static constexpr Ret jump(Method, void *t, Args... args) noexcept {
    static_assert(noexcept(extend(Method{}, *static_cast<T *>(t),
                                  std::forward<Args>(args)...)),
                  "Ret extend(Method, T&, Args...) is not declared noexcept, "
                  "but method is specified noexcept");
    using poly::extend;
    return extend(Method{}, *static_cast<T *>(t), std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) const noexcept> {
  template <typename T>
  static constexpr Ret jump(Method, const void *t, Args... args) noexcept {
    static_assert(
        noexcept(extend(Method{}, *static_cast<const T *>(t),
                        std::forward<Args>(args)...)),
        "Ret extend(Method, const T&, Args...) is not declared noexcept, "
        "but method is specified noexcept");
    using poly::extend;
    return extend(Method{}, *static_cast<const T *>(t),
                  std::forward<Args>(args)...);
  }
};

/// Individual VTable entry
template <typename MethodSpec> struct VTableEntry;
/// VTable specialization for non const method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...)> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...)>::template jump<T>)) {}

  constexpr Ret operator()(Method m, void *t, Args... args) const {
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, void *t, Args...);
};

/// VTable specialization for const method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr Ret operator()(Method m, const void *t, Args... args) const {
    return *func(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, const void *t, Args...);
};

/// VTable specialization for non const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...)>::template jump<T>)) {}

  constexpr Ret operator()(Method m, void *t, Args... args) const noexcept {
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, void *t, Args...) noexcept;
};

/// VTable specialization for const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr Ret operator()(Method m, const void *t,
                           Args... args) const noexcept {
    return *func(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, const void *t, Args...) noexcept;
};

/// complete vtable for a set of method specs
template <typename... MethodSpec>
struct VTable : private VTableEntry<MethodSpec>... {
  using VTableEntry<MethodSpec>::operator()...;
  template <typename T>
  constexpr VTable(poly::traits::Id<T> id) noexcept
      : VTableEntry<MethodSpec>(id)... {}
};

template <typename T, typename... MethodSpecs>
inline constexpr VTable<MethodSpecs...> vtable_for =
    VTable<MethodSpecs...>(poly::traits::Id<T>{});
} // namespace detail
} // namespace poly
#endif // !POLY_VTABLE_HPP
