#ifndef POLY_OBJECT_PROPERTY_HPP
#define POLY_OBJECT_PROPERTY_HPP
#include "poly/config.hpp"
#include "poly/property.hpp"
#include "poly/traits.hpp"

#include <utility>

namespace poly {
namespace detail {
using property_offset_type =
    traits::smallest_uint_to_contain<POLY_MAX_PROPERTY_COUNT>;

/// type trait to detect inner template "injector" in the name of the
/// PropertySpec
/// @{
template <POLY_PROP_SPEC PropertySpec, typename Self, typename = void>
struct has_injector : std::false_type {};
template <POLY_PROP_SPEC PropertySpec, typename Self>
struct has_injector<PropertySpec, Self,
                    std::void_t<typename property_name_t<
                        PropertySpec>::template injector<Self, PropertySpec>>>
    : std::true_type {};
/// @}

/// The PropertyInjector class eitheris an empty base class, or inherits from
/// the property names inner template "injector"
/// @{
/// default injector does nothing
template <POLY_PROP_SPEC PropertySpec, typename Self, bool hasInjector = false>
struct PropertyInjector {};

/// if the Property was created with the POLY_PROPERTY macro, the name is
/// injected and the property can be set with obj.PropertyName = value and
/// retrieved with obj.PropertyName
template <POLY_PROP_SPEC PropertySpec, typename Self>
struct PropertyInjector<PropertySpec, Self, true>
    : public property_name_t<PropertySpec>::template injector<Self,
                                                              PropertySpec> {
  using Base =
      typename property_name_t<PropertySpec>::template injector<Self,
                                                                PropertySpec>;
};

template <POLY_PROP_SPEC PropertySpec, typename Self>
using property_injector_for_t =
    PropertyInjector<PropertySpec, Self,
                     has_injector<PropertySpec, Self>::value>;
/// @}

/// The type of the injected member. It provides a conversion operator to Type
/// (i.e. get()), and an assignemnt operator for a T convertible to Type if the
/// property is not const (i.e. set()).
///
/// @tparam Self the type of the Object,
/// @tparam Injector the type containing the InjectedProperty as a member.
/// @tparam Name the name of the property
/// @tparam Type the value type of the property
/// @tparam Const specifies wether this property is const or not
/// @tparam NoThrow specifies wether this property is non throwing or not
/// @{
template <typename Self, typename Injector, typename Name, typename Type,
          bool Const, bool NoThrow>
class InjectedProperty {
public:
  using type = Type;

  operator Type() noexcept(NoThrow) {
    static_assert(not NoThrow or noexcept(self().template get<Name>()));
    return self().template get<Name>();
  }

  template <typename T, typename = std::enable_if_t<not std::is_same_v<
                            InjectedProperty, std::decay_t<T>>>>
  InjectedProperty &operator=(T &&t) noexcept(NoThrow) {
    self().template set<Name>(std::forward<T>(t));
    return *this;
  }

private:
  Self &self() noexcept {
    // reinterpret_cast is valid and not UB. InjectedProperty is the only member
    // of Names injector. If injector is standard_layout, the this pointer
    // (Injector*) is interconvertible with the pointer to the first member
    // (this). That the PropertyInjector is standard_layout, and as such all
    // members of it, is statically asserted in the PropertyContainer.
    return *static_cast<Self *>(reinterpret_cast<Injector *>(this));
  }
  const Self &self() const noexcept {
    // reinterpret_cast is valid and not UB. InjectedProperty is the only member
    // of Names injector. If injector is standard_layout, the this pointer
    // (Injector*) is interconvertible with the pointer to the first member
    // (this). That the PropertyInjector is standard_layout, and as such all
    // members of it, is statically asserted in the PropertyContainer.
    return *static_cast<const Self *>(reinterpret_cast<const Injector *>(this));
  }
};

template <typename Self, typename Injector, typename Name, typename Type,
          bool NoThrow>
class InjectedProperty<Self, Injector, Name, Type, true, NoThrow> {
public:
  using type = Type;

  template <typename S = Self, typename I = Injector>
  operator Type() noexcept(NoThrow) {
    static_assert(not NoThrow or noexcept(self().template get<Name>()));
    return self().template get<Name>();
  }

private:
  const Self &self() const noexcept {
    // reinterpret_cast is valid and not UB. InjectedProperty is the only member
    // of Names injector. If injector is standard_layout, the this pointer
    // (Injector*) is interconvertible with the pointer to the first member
    // (this). That the PropertyInjector is standard_layout, and as such all
    // members of it, is statically asserted in the PropertyContainer.
    return *static_cast<const Self *>(reinterpet_cast<const Injector *>(this));
  }
};
/// @}
/// @addtogroup ptable Property Table
/// Properties are accessed through a property table. It is a table of
/// individual ptable entries. The entries hold the type erased getter, and
/// optional setter, for a property. A pointer to the property table is held by
/// the PropertyContainer.
/// @{

/// Individual entry in the property table. Contains getter and optional
/// setter.
/// @{
template <POLY_PROP_SPEC PropertySpec> struct PTableEntry;
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

  constexpr Type get(Name, const void *t) const {
    assert(get_);
    assert(t);
    return (*get_)(Name{}, t);
  }

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
    assert(get_);
    assert(t);
    return (*get_)(Name{}, t);
  }

  Type (*get_)(Name, const void *);
};
template <typename Name, typename Type>
struct PTableEntry<Name(Type)> : PTableEntry<const Name(Type)> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T> id) noexcept
      : PTableEntry<const Name(Type)>(id),
        set_{+[](Name, void *t, const Type &value) -> bool {
          using poly::set;
          if constexpr (has_validator_v<T, Name(Type)>) {
            using poly::check;
            if (!check(Name{}, *static_cast<const T *>(t), value)) {
              return false;
            }
          }
          set(Name{}, *static_cast<T *>(t), value);
          return true;
        }} {}
  using PTableEntry<const Name(Type)>::get;
  constexpr bool set(Name, void *t, const Type &value) const {
    assert(set_);
    assert(t);
    return (*set_)(Name{}, t, value);
  }

  bool (*set_)(Name, void *, const Type &);
};
template <typename Name, typename Type>
struct PTableEntry<Name(Type) noexcept>
    : PTableEntry<const Name(Type) noexcept> {
  template <typename T>
  constexpr PTableEntry(poly::traits::Id<T> id) noexcept
      : PTableEntry<const Name(Type)>(id),
        set_{+[](Name, void *t, const Type &value) -> bool {
          using poly::set;
          static_assert(
              noexcept(set(std::declval<Name>(), std::declval<T &>())),
              "Property specified noexcept, but set(Name, T&) is "
              "not noexcept");
          if constexpr (has_validator_v<T, Name(Type)>) {
            using poly::check;
            static_assert(noexcept(check(Name{}, std::declval<const T &>(),
                                         std::declval<const Type &>())),
                          "Property specified noexcept, but check(Name, const "
                          "T&,const Type&) is not specified noexcept.");
            if (!check(Name{}, *static_cast<const T *>(t), value)) {
              return false;
            }
          }
          set(Name{}, *static_cast<T *>(t), value);
          return true;
        }} {}
  using PTableEntry<const Name(Type)>::get;
  constexpr bool set(Name, void *t, const Type &value) const noexcept {
    assert(set_);
    assert(t);
    return (*set_)(Name{}, t, value);
  }

  void (*set_)(Name, void *, const Type &);
};
/// @}

/// table of ptable entries properties
template <POLY_PROP_SPEC... PropertySpec>
struct PTable : private PTableEntry<PropertySpec>... {
  using PTableEntry<PropertySpec>::set...;
  using PTableEntry<PropertySpec>::get...;

  template <typename T>
  constexpr PTable(poly::traits::Id<T> id) : PTableEntry<PropertySpec>(id)... {}
  template <POLY_PROP_SPEC Spec>
  static constexpr property_offset_type offset(traits::Id<Spec>) noexcept {
    constexpr PTable<PropertySpec...> t;
    const std::byte *this_ =
        static_cast<const std::byte *>(static_cast<const void *>(&t));
    const std::byte *entry_ = static_cast<const std::byte *>(
        static_cast<const void *>(static_cast<const PTableEntry<Spec> *>(&t)));
    return static_cast<property_offset_type>(entry_ - this_);
  }
};

template <typename T, POLY_PROP_SPEC... PropertySpecs>
inline constexpr PTable<PropertySpecs...> ptable_for =
    PTable<PropertySpecs...>(poly::traits::Id<T>{});

/// returns the Spec in Specs belonging to Name
template <typename Name, POLY_PROP_SPEC... Specs> struct spec_by_name {
  template <typename T>
  using predicate = std::is_same<property_name_t<T>, Name>;
  using type = at_t<filter_t<type_list<Specs...>, predicate>, 0>;
};

/// This class holds a pointer to a property table, if the provided list of
/// PropertySpecs is not empty. It provides the get and set method for
/// properties, as well as access to injected properties by name.
/// @{
/// @tparam ListOfPropertySpecs TypeList of PropertySpec
/// "PropertySpecs".
template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
class PropertyContainer;

template <typename Self, template <typename...> typename List,
          POLY_PROP_SPEC... PropertySpecs>
class PropertyContainer<Self, List<PropertySpecs...>>
    : public property_injector_for_t<PropertySpecs, Self>... {
  static_assert(
      (poly::traits::is_property_spec_v<PropertySpecs> && ...),
      "The provided PropertySpecs must be valid PropertySpecs, i.e. a funcion "
      "type with signature [const] PropertyName(Type) [noexcept].");
  template <typename Name>
  using spec_for = typename spec_by_name<Name, PropertySpecs...>::type;
  template <typename Name> using value_type_for = value_type_t<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

  template <typename, POLY_TYPE_LIST> friend class InterfacePropertyContainer;

  static_assert((
      std::is_standard_layout_v<property_injector_for_t<PropertySpecs, Self>> &&
      ...));

public:
  constexpr PropertyContainer(Self &self) noexcept
      : property_injector_for_t<PropertySpecs, Self>(self)... {}

  constexpr PropertyContainer(const PropertyContainer &) = default;
  constexpr PropertyContainer(PropertyContainer &&) = default;
  constexpr PropertyContainer &operator=(const PropertyContainer &) = default;
  constexpr PropertyContainer &operator=(PropertyContainer &&) = default;

  template <typename Name, typename = std::enable_if_t<not is_const<Name>>>
  constexpr bool
  set(const value_type_for<Name> &value) noexcept(is_nothrow<Name>) {
    return ptable_->set(Name{}, self().data(), value);
  }
  template <typename Name>
  constexpr value_type_for<Name> get() const noexcept(is_nothrow<Name>) {
    return ptable_->get(Name{}, self().data());
  }

protected:
  template <typename T> constexpr void set_ptable(traits::Id<T>) noexcept {
    ptable_ = &ptable_for<T, PropertySpecs...>;
  }

  constexpr void set_ptable(const PTable<PropertySpecs...> *ptable) noexcept {
    ptable_ = ptable;
  }

  constexpr const PTable<PropertySpecs...> *ptable() const noexcept {
    return ptable_;
  }

  constexpr void reset_ptable() noexcept { ptable_ = nullptr; }

private:
  constexpr Self &self() noexcept { return *static_cast<Self *>(this); }
  constexpr const Self &self() const noexcept {
    return *static_cast<const Self *>(this);
  }
  const PTable<PropertySpecs...> *ptable_{nullptr};
};
/// specialization for empty list of @ref PropertySpec "PropertySpecs"
template <typename Self, template <typename...> typename List>
class PropertyContainer<Self, List<>> {
  constexpr PropertyContainer(Self &) noexcept {}

protected:
  template <typename T> constexpr void set_ptable(traits::Id<T>) noexcept {}

  constexpr void set_ptable(const void *ptable) noexcept {}

  constexpr const void *ptable() const noexcept { return nullptr; }

  constexpr void reset_ptable() noexcept {}
};

} // namespace detail
} // namespace poly
#endif
