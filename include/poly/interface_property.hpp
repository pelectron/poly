#ifndef POLY_INTERFACE_PROPERTY_HPP
#define POLY_INTERFACE_PROPERTY_HPP
#include "poly/object_property.hpp"
namespace poly::detail {

template <POLY_PROP_SPEC PropertySpec> struct InterfacePTableEntry;
template <typename Name, typename Type>
struct InterfacePTableEntry<const Name(Type)> {
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;

  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *) noexcept
      : offset{PTable<Specs...>::offset(traits::Id<const Name(Type)>{})} {}

  template <typename T> constexpr void set(Name, void *, const T &) const {
    static_assert(detail::always_false<T>,
                  "This property is not settable, i.e. defined as const.");
  }

  Type get(Name, const void *table, const void *t) const {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<const Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->get(Name{}, t);
  }
  property_offset_type offset{0};
};

template <typename Name, typename Type>
struct InterfacePTableEntry<const Name(Type) noexcept> {
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  InterfacePTableEntry(const PTable<Specs...> *) noexcept
      : offset{PTable<Specs...>::offset(
            traits::Id<const Name(Type) noexcept>{})} {}

  template <typename T> void set(Name, void *, const T &) const noexcept {
    static_assert(detail::always_false<T>,
                  "This property is not settable, i.e. defined as const.");
  }

  Type get(Name, const void *table, const void *t) const noexcept {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<const Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->get(Name{}, t);
  }
  property_offset_type offset{0};
};

template <typename Name, typename Type>
struct InterfacePTableEntry<Name(Type)>
    : private InterfacePTableEntry<const Name(Type)> {
  using InterfacePTableEntry<const Name(Type)>::get;
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *p) noexcept
      : InterfacePTableEntry<const Name(Type)>(p),
        offset{PTable<Specs...>::offset(traits::Id<const Name(Type)>{})} {}
  bool set(Name, const void *table, void *t, const Type &value) const {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<const Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->set(Name{}, t, value);
  }

  property_offset_type offset{0};
};

template <typename Name, typename Type>
struct InterfacePTableEntry<Name(Type) noexcept>
    : private InterfacePTableEntry<const Name(Type) noexcept> {
  using InterfacePTableEntry<const Name(Type) noexcept>::get;
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *p) noexcept
      : InterfacePTableEntry<const Name(Type)>(p),
        offset{PTable<Specs...>::offset(traits::Id<const Name(Type)>{})} {}
  bool set(Name, const void *table, void *t, const Type &value) const noexcept {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<const Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->set(Name{}, t, value);
  }

  property_offset_type offset{0};
};

template <POLY_PROP_SPEC... PropertySpecs>
class InterfacePTable : private InterfacePTableEntry<PropertySpecs>... {
public:
  using InterfacePTableEntry<PropertySpecs>::set...;
  using InterfacePTableEntry<PropertySpecs>::get...;

  template <typename Name>
  using spec_for = typename spec_by_name<Name, PropertySpecs...>::type;
  template <typename Name> using value_type_for = value_type_t<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

  template <POLY_PROP_SPEC... Specs>
  InterfacePTable(const PTable<Specs...> *table)
      : InterfacePTableEntry<PropertySpecs>(table)..., table_(table) {}
  InterfacePTable(const InterfacePTable &) = default;
  InterfacePTable(InterfacePTable &&) = default;
  InterfacePTable &operator=(const InterfacePTable &) = default;
  InterfacePTable &operator=(InterfacePTable &&) = default;
  template <typename Name>
  bool set(Name, void *obj,
           const value_type_for<Name> &value) noexcept(is_nothrow<Name>) {
    assert(obj);
    assert(table_);
    return this->set(Name{}, table_, obj, value);
  }

  template <typename Name>
  value_type_for<Name> get(Name, const void *obj) noexcept(is_nothrow<Name>) {
    assert(obj);
    assert(table_);
    return this->get(Name{}, table_, obj);
  }

private:
  void *table_;
};

template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
class InterfacePropertyContainer;

template <typename Self, template <typename...> typename List,
          POLY_PROP_SPEC... PropertySpecs>
class InterfacePropertyContainer<Self, List<PropertySpecs...>>
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

  template <typename, typename, typename> friend class basic_interface;

public:
  constexpr InterfacePropertyContainer(const InterfacePropertyContainer &) =
      default;
  constexpr InterfacePropertyContainer(InterfacePropertyContainer &&) = default;
  constexpr InterfacePropertyContainer &
  operator=(const InterfacePropertyContainer &) = default;
  constexpr InterfacePropertyContainer &
  operator=(InterfacePropertyContainer &&) = default;
  template <POLY_PROP_SPEC... Specs>
  InterfacePropertyContainer(Self &self, const PTable<Specs...> *table) noexcept
      : property_injector_for_t<PropertySpecs, Self>(self)..., ptable_(table) {}

  template <typename Name, typename = std::enable_if_t<not is_const<Name>>>
  bool set(const value_type_for<Name> &value) noexcept(is_nothrow<Name>) {
    return ptable_.set(Name{}, self().data(), value);
  }

  template <typename Name>
  value_type_for<Name> get() const noexcept(is_nothrow<Name>) {
    return ptable_.get(Name{}, self().data());
  }

private:
  constexpr Self &self() noexcept { return *static_cast<Self *>(this); }
  constexpr const Self &self() const noexcept {
    return *static_cast<const Self *>(this);
  }
  InterfacePTable<PropertySpecs...> ptable() const noexcept { return ptable_; }
  void ptable(const InterfacePTable<PropertySpecs...> &table) const noexcept {
    ptable_ = table;
  }
  InterfacePTable<PropertySpecs...> ptable_;
};

template <typename Self, template <typename...> typename List>
class InterfacePropertyContainer<Self, List<>> {
public:
  constexpr InterfacePropertyContainer(Self &, const void *) noexcept {}
};
} // namespace poly::detail
#endif
