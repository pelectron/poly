#ifndef POLY_PROPERTY_HPP
#define POLY_PROPERTY_HPP
#include "poly/config.hpp"

#include <utility> // needed for std::declval

#if POLY_USE_MACROS == 1
#if (POLY_USE_INJECTOR == 1) || (POLY_USE_PROPERTY_INJECTOR == 1)
#define POLY_PROPERTY_IMPL(name)                                               \
  struct name {                                                                \
    template <typename Self, typename Spec> struct injector;                   \
    template <typename Self, typename Type>                                    \
    struct injector<Self, name(Type)> {                                        \
      Type name() const {                                                      \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template get<struct name>();                              \
      }                                                                        \
      void name(const Type &value) {                                           \
        Self *self = static_cast<Self *>(this);                                \
        self->template set<struct name>(value);                                \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Type>                                    \
    struct injector<Self, name(Type) noexcept> {                               \
      Type name() const noexcept {                                             \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template get<struct name>();                              \
      }                                                                        \
      void name(const Type &value) noexcept {                                  \
        Self *self = static_cast<Self *>(this);                                \
        self->template set<struct name>(value);                                \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Type>                                    \
    struct injector<Self, const name(Type)> {                                  \
      Type name() const {                                                      \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template get<struct name>();                              \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Type>                                    \
    struct injector<Self, const name(Type) noexcept> {                         \
      Type name() const noexcept {                                             \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template get<struct name>();                              \
      }                                                                        \
    };                                                                         \
  };
#else
#define POLY_PROPERTY_IMPL(name)                                               \
  struct name {};
#endif

#if POLY_USE_DEFAULT_PROPERTY_ACCESS == 1
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

namespace poly {

/// getter for PropertySpec '[const]PropertyName(Type)[noexcept]'
template <typename Type, typename PropertyName, typename T>
Type get(PropertyName, const T &t);

/// setter for PropertySpec 'PropertyName(Type)[noexcept]'
template <typename PropertyName, typename T, typename Type>
void set(PropertyName, T &t, const Type &value);

namespace detail {
/// default injector does nothing
template <typename PropertySpec, typename Self, typename = void>
struct PropertyInjector {};

/// if the Property was created with the POLY_PROPERTY macro, the name is
/// injected and the property can be set with obj.PropertyName(value) and
/// retrieved with obj.PropertyName()
template <typename Name, typename Type, typename Self>
struct PropertyInjector<
    Name(Type), Self,
    std::void_t<typename Name::template injector<Self, Name(Type)>>>
    : Name::template injector<Self, Name(Type)> {};

template <typename Name, typename Type, typename Self>
struct PropertyInjector<
    const Name(Type), Self,
    std::void_t<typename Name::template injector<Self, const Name(Type)>>>
    : Name::template injector<Self, const Name(Type)> {};

template <typename Name, typename Type, typename Self>
struct PropertyInjector<
    Name(Type) noexcept, Self,
    std::void_t<typename Name::template injector<Self, Name(Type) noexcept>>>
    : Name::template injector<Self, Name(Type) noexcept> {};

template <typename Name, typename Type, typename Self>
struct PropertyInjector<const Name(Type) noexcept, Self,
                        std::void_t<typename Name::template injector<
                            Self, const Name(Type) noexcept>>>
    : Name::template injector<Self, const Name(Type) noexcept> {};

} // namespace detail
} // namespace poly
#endif
