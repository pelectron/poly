#ifndef POLY_PROPERTY_HPP
#define POLY_PROPERTY_HPP
#include "poly/always_false.hpp"
#include "poly/config.hpp"
#include "poly/traits.hpp"

#include <utility>

#if POLY_USE_MACROS == 1
#if (POLY_USE_INJECTOR == 1) || (POLY_USE_PROPERTY_INJECTOR == 1)
#define POLY_PROPERTY_IMPL(name)                                               \
  struct name {                                                                \
    template <typename Self, typename Spec> struct injector {                  \
      constexpr injector(Self &self) noexcept : name(self) {}                  \
      constexpr injector(const injector &other) noexcept : name(other.name) {} \
      constexpr injector(injector &&other) noexcept                            \
          : name(std::move(other.name)) {}                                     \
      poly::detail::InjectedProperty<                                          \
          Self, poly::property_name_t<Spec>, poly::value_type_t<Spec>,         \
          poly::is_const_property_v<Spec>, poly::is_nothrow_property_v<Spec>>  \
          name;                                                                \
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
/// @addtogroup property_traits
/// @{

/// getter for @ref property_traits "PropertySpec"
/// '[const]PropertyName(Type)[noexcept]'.
///
/// This function needs to be defined for a T to implement a specific
/// PropertySpec. For nothrow PropertySpecs, the get() function must be
/// noexcept.
///
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename Type, typename PropertyName, typename T>
Type get(PropertyName, const T &t);

/// setter for @ref property_traits "PropertySpec"
/// 'PropertyName(Type)[noexcept]'.
///
/// This function needs to be defined for a T to implement a specific
/// PropertySpec. For nothrow PropertySpecs, the set() function must be
/// noexcept.
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename PropertyName, typename T, typename Type>
void set(PropertyName, T &t, const Type &value);

/// optional checker for @ref property_traits "PropertySpec"
/// '[const]PropertyName(Type)[noexcept]'.
///
/// This function can be defined for enable validation before setting properties
/// on a T. If the function returns false, the value is not set, i.e
/// set(PropertyName,T&,const Type&) is not called. Calling
/// interface::set<PropertyName>(value) returns the result of calling check().
///
/// @note when using the assignment operator on injected properties, the
/// value is set as described above, but return value of check() cannot be
/// retrieved by the caller.
///
/// @tparam Type the value type of the Property
/// @tparam PropertyName the name of the Property
/// @tparam T the of the objec the property belongs to
template <typename PropertyName, typename T, typename Type,
          typename = std::enable_if_t<detail::always_false<T>>>
bool check(PropertyName, const T &t, const Type &value);
/// @}

namespace detail {
/// type trait to detect inner template "injector" in the name of the
/// PropertySpec
/// @{
template <typename PropertySpec, typename Self, typename = void>
struct has_injector : std::false_type {};
template <typename PropertySpec, typename Self>
struct has_injector<PropertySpec, Self,
                    std::void_t<typename property_name_t<
                        PropertySpec>::template injector<Self, PropertySpec>>>
    : std::true_type {};
/// @}

/// The PropertyInjector class eitheris an empty base class, or inherits from
/// the property names inner template "injector"
/// @{
/// default injector does nothing
template <typename PropertySpec, typename Self, bool hasInjector = false>
struct PropertyInjector {
  constexpr PropertyInjector(Self &) noexcept {}
  constexpr PropertyInjector(const PropertyInjector &) noexcept = default;
  constexpr PropertyInjector(PropertyInjector &&) noexcept = default;
  constexpr PropertyInjector &
  operator=(const PropertyInjector &) noexcept = default;
  constexpr PropertyInjector &operator=(PropertyInjector &&) noexcept = default;
};
/// if the Property was created with the POLY_PROPERTY macro, the name is
/// injected and the property can be set with obj.PropertyName = value and
/// retrieved with obj.PropertyName
template <typename PropertySpec, typename Self>
struct PropertyInjector<PropertySpec, Self, true>
    : public property_name_t<PropertySpec>::template injector<Self,
                                                              PropertySpec> {
  using Base =
      typename property_name_t<PropertySpec>::template injector<Self,
                                                                PropertySpec>;
  constexpr PropertyInjector(Self &self) noexcept : Base(self) {}
  constexpr PropertyInjector(const PropertyInjector &other) noexcept
      : Base(other.self) {}
  constexpr PropertyInjector(PropertyInjector &&other) noexcept
      : Base(other.self) {}
};

template <typename PropertySpec, typename Self>
using property_injector_for_t =
    PropertyInjector<PropertySpec, Self,
                     has_injector<PropertySpec, Self>::value>;
/// @}

/// The type of the injected member. It provides a conversion operator to Type
/// (i.e. get()), and an assignemnt operator for a T convertible to Type if the
/// property is not const (i.e. set()).
/// @{
template <typename Self, typename Name, typename Type, bool Const, bool NoThrow>
class InjectedProperty {
public:
  using type = Type;
  constexpr InjectedProperty(Self &self) noexcept : self(self) {}
  constexpr InjectedProperty(const InjectedProperty &other) noexcept
      : self(other.self) {}
  constexpr InjectedProperty(InjectedProperty &&other) noexcept
      : self(other.self) {}

  constexpr operator Type() noexcept(NoThrow) {
    static_assert(not NoThrow or noexcept(self.template get<Name>()));
    return self.template get<Name>();
  }
  template <typename T>
  constexpr InjectedProperty &operator=(T &&t) noexcept(NoThrow) {
    self.template set<Name>(std::forward<T>(t));
    return *this;
  }

private:
  Self &self;
};

template <typename Self, typename Name, typename Type, bool NoThrow>
class InjectedProperty<Self, Name, Type, true, NoThrow> {
public:
  using type = Type;

  constexpr InjectedProperty(Self &self) noexcept : self(self) {}

  constexpr operator Type() noexcept(NoThrow) {
    static_assert(not NoThrow or noexcept(self.template get<Name>()));
    return self.template get<Name>();
  }

private:
  Self &self;
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
template <typename... PropertySpec>
struct PTable : private PTableEntry<PropertySpec>... {
  using PTableEntry<PropertySpec>::set...;
  using PTableEntry<PropertySpec>::get...;

  template <typename T>
  constexpr PTable(poly::traits::Id<T> id) : PTableEntry<PropertySpec>(id)... {}
};

template <typename T, typename... PropertySpecs>
inline constexpr PTable<PropertySpecs...> ptable_for =
    PTable<PropertySpecs...>(poly::traits::Id<T>{});

/// returns the Spec in Specs belonging to Name
template <typename Name, typename... Specs> struct spec_by_name {
  template <typename T>
  using predicate = std::is_same<property_name_t<T>, Name>;
  using type = at_t<filter_t<type_list<Specs...>, predicate>, 0>;
};

/// This class holds a pointer to a property table, if the provided list of
/// PropertySpecs is not empty. It provides the get and set method for
/// properties, as well as access to injected properties by name.
/// @{
/// @tparam ListOfPropertySpecs poly::type_list of @ref property_traits
/// "PropertySpecs".
template <typename Self, typename ListOfPropertySpecs> class PropertyContainer;

template <typename Self, typename... PropertySpecs>
class PropertyContainer<Self, type_list<PropertySpecs...>>
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

public:
  constexpr PropertyContainer(Self &self) noexcept
      : property_injector_for_t<PropertySpecs, Self>(self)... {}

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
  const PTable<PropertySpecs...> *ptable_;
};
/// specialization for empty list of @ref property_traits "PropertySpecs"
template <typename Self> class PropertyContainer<Self, type_list<>> {
  constexpr PropertyContainer(Self &) noexcept {}

protected:
  template <typename T> constexpr void set_ptable(traits::Id<T>) noexcept {}

  constexpr void set_ptable(const void *ptable) noexcept {}

  constexpr const void *ptable() const noexcept { return nullptr; }

  constexpr void reset_ptable() noexcept {}
};
/// @}
/// @}
} // namespace detail
} // namespace poly
#endif
