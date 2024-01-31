#ifndef POLY_INTERFACE_PROPERTY_HPP
#define POLY_INTERFACE_PROPERTY_HPP
#include "poly/object_property.hpp"
namespace poly::detail {

#define POLY_STRINGIFY(x) POLY_STRINGIFY1(x)
#define POLY_STRINGIFY1(x) #x

/// @addtogroup ptable
/// @{

/// An individual entry in the interface property table
/// @{
template <POLY_PROP_SPEC PropertySpec> class InterfacePTableEntry;
template <typename Name, typename Type>
class InterfacePTableEntry<const Name(Type)> {
public:
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;

  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *) noexcept
      : offset{PTable<Specs...>::offset(traits::Id<const Name(Type)>{})} {
    static_assert(
        sizeof...(Specs) <= POLY_MAX_PROPERTY_COUNT,
        "Error while creating an Interface from an Object: the number of "
        "properties in the Object exceeds POLY_MAX_PROPERTY_COUNT "
        "(=" POLY_STRINGIFY(
            POLY_MAX_PROPERTY_COUNT) "). Adjust the POLY_MAX_PROPERTY_COUNT "
                                     "macro to reflect "
                                     "the maximum number of properties "
                                     "accurately before "
                                     "including ANY poly header.");
  }

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

private:
  property_offset_type offset{0};
};

template <typename Name, typename Type>
class InterfacePTableEntry<const Name(Type) noexcept> {
public:
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  InterfacePTableEntry(const PTable<Specs...> *) noexcept
      : offset{
            PTable<Specs...>::offset(traits::Id<const Name(Type) noexcept>{})} {
    static_assert(
        sizeof...(Specs) <= POLY_MAX_PROPERTY_COUNT,
        "Error while creating an Interface from an Object: the number of "
        "properties in the Object exceeds POLY_MAX_PROPERTY_COUNT "
        "(=" POLY_STRINGIFY(
            POLY_MAX_PROPERTY_COUNT) "). Adjust the POLY_MAX_PROPERTY_COUNT "
                                     "macro to reflect "
                                     "the maximum number of properties "
                                     "accurately before "
                                     "including ANY poly header.");
  }

  template <typename T> void set(Name, void *, const T &) const noexcept {
    static_assert(detail::always_false<T>,
                  "This property is not settable, i.e. defined as const.");
  }

  Type get(Name, const void *table, const void *t) const noexcept {
    assert(table);
    assert(t);
    const auto *entry = static_cast<const PTable<const Name(Type) noexcept> *>(
        static_cast<const void *>(static_cast<const std::byte *>(table) +
                                  offset));
    return entry->get(Name{}, t);
  }

private:
  property_offset_type offset{0};
};

template <typename Name, typename Type> class InterfacePTableEntry<Name(Type)> {
public:
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *p) noexcept
      : offset{PTable<Specs...>::offset(traits::Id<Name(Type)>{})} {
    static_assert(
        sizeof...(Specs) <= POLY_MAX_PROPERTY_COUNT,
        "Error while creating an Interface from an Object: the number of "
        "properties in the Object exceeds POLY_MAX_PROPERTY_COUNT "
        "(=" POLY_STRINGIFY(
            POLY_MAX_PROPERTY_COUNT) "). Adjust the POLY_MAX_PROPERTY_COUNT "
                                     "macro to reflect "
                                     "the maximum number of properties "
                                     "accurately before "
                                     "including ANY poly header.");
  }
  bool set(Name, const void *table, void *t, const Type &value) const {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->set(Name{}, t, value);
  }

  Type get(Name, const void *table, const void *t) const {
    assert(table);
    assert(t);
    const auto *entry =
        static_cast<const PTable<Name(Type)> *>(static_cast<const void *>(
            static_cast<const std::byte *>(table) + offset));
    return entry->get(Name{}, t);
  }

private:
  property_offset_type offset{0};
};

template <typename Name, typename Type>
class InterfacePTableEntry<Name(Type) noexcept> {
public:
  InterfacePTableEntry(const InterfacePTableEntry &) = default;
  InterfacePTableEntry(InterfacePTableEntry &&) = default;
  InterfacePTableEntry &operator=(const InterfacePTableEntry &) = default;
  InterfacePTableEntry &operator=(InterfacePTableEntry &&) = default;
  template <POLY_PROP_SPEC... Specs>
  constexpr InterfacePTableEntry(const PTable<Specs...> *p) noexcept
      : offset{PTable<Specs...>::offset(traits::Id< Name(Type)noexcept>{})} {
    static_assert(
        sizeof...(Specs) <= POLY_MAX_PROPERTY_COUNT,
        "Error while creating an Interface from an Object: the number of "
        "properties in the Object exceeds POLY_MAX_PROPERTY_COUNT "
        "(=" POLY_STRINGIFY(
            POLY_MAX_PROPERTY_COUNT) "). Adjust the POLY_MAX_PROPERTY_COUNT "
                                     "macro to reflect "
                                     "the maximum number of properties "
                                     "accurately before "
                                     "including ANY poly header.");
  }

  bool set(Name, const void *table, void *t, const Type &value) const noexcept {
    assert(table);
    assert(t);
    const auto *entry = static_cast<const PTable<Name(Type) noexcept> *>(
        static_cast<const void *>(static_cast<const std::byte *>(table) +
                                  offset));
    return entry->set(Name{}, t, value);
  }

  Type get(Name, const void *table, const void *t) const {
    assert(table);
    assert(t);
    const auto *entry = static_cast<const PTable<Name(Type) noexcept> *>(
        static_cast<const void *>(static_cast<const std::byte *>(table) +
                                  offset));
    return entry->get(Name{}, t);
  }

private:
  property_offset_type offset{0};
};

/// @}

/// A complete interface property table.
template <POLY_PROP_SPEC... PropertySpecs>
class POLY_EMPTY_BASE InterfacePTable
    : public InterfacePTableEntry<PropertySpecs>... {
public:
  template <typename Name>
  using spec_for = typename spec_by_name<Name, PropertySpecs...>::type;
  template <typename Name> using value_type_for = value_type_t<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_nothrow = is_nothrow_property_v<spec_for<Name>>;
  template <typename Name>
  static constexpr bool is_const = is_const_property_v<spec_for<Name>>;

  template <POLY_PROP_SPEC... Specs>
  InterfacePTable(const InterfacePTable<Specs...> &other) noexcept
      : InterfacePTableEntry<PropertySpecs>(other)..., table_(other.table_) {}

  template <POLY_PROP_SPEC... Specs>
  InterfacePTable(InterfacePTable<Specs...> &&other) noexcept
      : InterfacePTableEntry<PropertySpecs>(other)..., table_(other.table_) {}

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
  const void *table_;
};

/// contains either an InterfacePTable, or is empty if the ListOfPropertySpecs
/// is empty. Provides set and get for properties of an Interface, as well as
/// the injected rpoerties, if there are any.
/// @{
template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
class POLY_EMPTY_BASE InterfacePropertyContainer;

template <typename Self, template <typename...> typename List,
          POLY_PROP_SPEC... PropertySpecs>
class POLY_EMPTY_BASE InterfacePropertyContainer<Self, List<PropertySpecs...>>
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
  template <typename S, typename SpecList>
  InterfacePropertyContainer(
      const InterfacePropertyContainer<S, SpecList> &other) noexcept
      : ptable_(other.ptable_) {}
  template <typename S, typename SpecList>
  InterfacePropertyContainer(
      InterfacePropertyContainer<S, SpecList> &&other) noexcept
      : ptable_(std::move(other.ptable_)) {}

  template <POLY_PROP_SPEC... Specs>
  InterfacePropertyContainer(const PTable<Specs...> *table) noexcept
      : ptable_(table) {}

  /// set the property with name Name to the value value.
  /// returns: true if value was set
  /// returns: false if the check failed
  /// @param value value to set the property to
  /// @tparam Name name of the property
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

/// specialization for empty PropertySpec list
template <typename Self, template <typename...> typename List>
class InterfacePropertyContainer<Self, List<>> {
public:
  constexpr InterfacePropertyContainer(Self &, const void *) noexcept {}
};
/// @}
#undef POLY_STRINGIFY
#undef POLY_STRINGIFY1
} // namespace poly::detail
#endif
