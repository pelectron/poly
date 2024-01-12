#ifndef POLY_PROPERTY_HPP
#define POLY_PROPERTY_HPP
#include <utility> // needed for std::declval

#define POLY_PROPERTY(name)                                                    \
  struct name;                                                                 \
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
  };                                                                           \
                                                                               \
  template <typename T>                                                        \
  auto get(name, const T &t) noexcept(std::is_nothrow_copy_constructible_v<    \
                                      decltype(std::declval<T>().name)>) {     \
    return t.name;                                                             \
  }                                                                            \
                                                                               \
  template <typename T, typename Type>                                         \
  void set(name, T &t, const Type &value) noexcept(                            \
      std::is_nothrow_copy_assignable_v<decltype(std::declval<T>().name)>) {   \
    t.name = value;                                                            \
  }

namespace poly {

/// getter for PropertySpec '[const]PropertyName(Type)[noexcept]'
template <typename Type, typename PropertyName, typename T>
Type get(PropertyName, const T &t);

/// setter for PropertySpec 'PropertyName(Type)[noexcept]'
template <typename PropertyName, typename T, typename Type>
void set(PropertyName, T &t, const Type &value);

} // namespace poly
#endif
