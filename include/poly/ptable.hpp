#ifndef POLY_PTABLE_HPP
#define POLY_PTABLE_HPP
#include "poly/always_false.hpp"
#include "poly/property.hpp"
#include "poly/traits.hpp"

namespace poly {

namespace detail {

/// Individual entry in the property table. Contains getter and optional
/// setter.
template <typename PropertySpec> struct PTableEntry;

template <typename Name, typename Type> struct PTableEntry<const Name(Type)> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T>) noexcept
      : get_(+[](Name, const void *t) -> Type {
          using poly::get;
          return get(Name{}, *static_cast<const T *>(t));
        }) {}

  template <typename T> constexpr void set(Name, void *, const T &) const {
    static_assert(detail::always_false<T>,
                  "This property is not settable, i.e. defined as const.");
  }

  constexpr Type get(Name, const void *t) const { return (*get_)(Name{}, t); }

  Type (*get_)(Name, const void *);
};

template <typename Name, typename Type>
struct PTableEntry<const Name(Type) noexcept> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T>) noexcept
      : get_(+[](Name, const void *t) noexcept -> Type {
          using poly::get;
          static_assert(
              noexcept(get(std::declval<Name>(), std::declval<const T>())),
              "Property specified noexcept, but get(Name, const T&)->Type is "
              "not noexcept");
          return get(Name{}, *static_cast<const T *>(t));
        }) {}

  template <typename T>
  constexpr void set(Name, void *, const T &) const noexcept {
    static_assert(
        detail::always_false<T>,
        "This property is not settable, i.e. it is defined as const.");
  }

  constexpr Type get(Name, const void *t) const noexcept {
    return (*get_)(Name{}, t);
  }

  Type (*get_)(Name, const void *);
};
template <typename Name, typename Type>
struct PTableEntry<Name(Type)> : PTableEntry<const Name(Type)> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T> id) noexcept
      : PTableEntry<const Name(Type)>(id),
        set_{+[](Name, void *t, const Type &value) -> void {
          using poly::set;
          set(Name{}, *static_cast<T *>(t), value);
        }} {}
  using PTableEntry<const Name(Type)>::get;
  constexpr void set(Name, void *t, const Type &value) const {
    return (*set_)(Name{}, t, value);
  }

  void (*set_)(Name, void *, const Type &);
};
template <typename Name, typename Type>
struct PTableEntry<Name(Type) noexcept>
    : PTableEntry<const Name(Type) noexcept> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T> id) noexcept
      : PTableEntry<const Name(Type)>(id),
        set_{+[](Name, void *t, const Type &value) -> void {
          using poly::set;
          static_assert(
              noexcept(set(std::declval<Name>(), std::declval<T &>())),
              "Property specified noexcept, but set(Name, T&) is "
              "not noexcept");
          set(Name{}, *static_cast<T *>(t), value);
        }} {}
  using PTableEntry<const Name(Type)>::get;
  constexpr void set(Name, void *t, const Type &value) const noexcept {
    return (*set_)(Name{}, t, value);
  }

  void (*set_)(Name, void *, const Type &);
};

/// table of getters and setter for properties
template <typename... PropertySpec>
struct PTable : private PTableEntry<PropertySpec>... {
  using PTableEntry<PropertySpec>::set...;
  using PTableEntry<PropertySpec>::get...;

  template <typename T>
  constexpr PTable(poly::traits::Id<T> id) : PTableEntry<PropertySpec>(id)... {}
};

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

template <typename T, typename... PropertySpecs>
inline constexpr PTable<PropertySpecs...> ptable_for =
    PTable<PropertySpecs...>(poly::traits::Id<T>{});

} // namespace detail
} // namespace poly
#endif
