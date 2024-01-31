#ifndef POLY_OBJECT_HPP
#define POLY_OBJECT_HPP
#include "poly/object_method.hpp"
#include "poly/object_property.hpp"
#include "poly/storage.hpp"

namespace poly {
namespace detail {
template <typename Storage, typename T, typename... Args>
inline constexpr bool nothrow_emplaceable_v = noexcept(
    std::declval<Storage>().template emplace<T>(std::declval<Args>()...));

}
/// @addtogroup object Objects
/// Objects (note the capital O) are the core class templates this library
/// provides.
///
/// Objects provide methods and properties described in their lists of @ref
/// MethodSpec "MethodSpecs" and @ref PropertySpec "PropertySpecs".
///
/// Properties model member variables and are defined by @ref PropertySpec
/// "property specs". These specs describe the access (const/mutable), name, and
/// value type of the property.
///
/// A property of an Object with name **n** can be accessed by the
/// Objects set<**n**> and get<**n**> member functions. If name
/// injection is enabled, and the name **n** is created with the
/// @ref POLY_PROPERTY macro, the property can be accessed as if
/// it were a normal c++ member variable of that Object, i.e. ``auto v = obj.n``
/// and ``obj.n = v`` instead of ``auto v = obj.get<n>()`` and
/// ``obj.set<n>(v)``.
///
/// Methods model member functions. They are invoked through an
/// Objects/Interfaces call member function.
///
/// @{

/// This class implements the polymorphic behaviour.
///
/// It holds objects in a storage of type StorageType, and provides the
/// properties and methods given by the list of PropertySpecs and MethodSpecs.
///
/// Objects can only be created and assigned to from types implementing the
/// properties and methods specified by the PropertySpecs and MethodSpecs, and
/// from other Objects with IDENTICAL PropertySpecs and MethodSpecs. Objects
/// cannot be created from Interfaces.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// poly::Storage concept.
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
/// @{
template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
class basic_object
    : public detail::PropertyContainer<
          basic_object<StorageType, PropertySpecs, MethodSpecs>, PropertySpecs>,
      public detail::MethodContainer<
          basic_object<StorageType, PropertySpecs, MethodSpecs>, MethodSpecs> {
public:
  using P = detail::PropertyContainer<
      basic_object<StorageType, PropertySpecs, MethodSpecs>, PropertySpecs>;
  using M = detail::MethodContainer<
      basic_object<StorageType, PropertySpecs, MethodSpecs>, MethodSpecs>;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::MethodContainerImpl;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::PropertyContainer;
  template <POLY_STORAGE S, POLY_TYPE_LIST P, POLY_TYPE_LIST M>
  friend class basic_interface;

  using method_specs = MethodSpecs;
  using property_specs = PropertySpecs;

  static_assert(poly::is_type_list_v<PropertySpecs>,
                "The PropertySpecs must be provided as a poly::type_list of "
                "PropertySpec.");
  static_assert(
      poly::is_type_list_v<MethodSpecs>,
      "The MethodSpecs must be provided as a poly::type_list of MethodSpec.");
  static_assert(poly::is_storage_v<StorageType>,
                "StorageType must satisfy the Storage concept.");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<MethodSpecs, traits::is_method_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<PropertySpecs, traits::is_property_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");

  constexpr basic_object() noexcept = default;

  /// copy ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  constexpr basic_object(
      const basic_object<OtherStorage, PropertySpecs, MethodSpecs> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}
  /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
  /// storage type)
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &>>>
  constexpr basic_object(
      basic_object<OtherStorage, PropertySpecs, MethodSpecs> &other) noexcept
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// move ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &&>>>
  constexpr basic_object(
      basic_object<OtherStorage, PropertySpecs, MethodSpecs>
          &&other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  constexpr basic_object(basic_object &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, StorageType &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}

  /// construct from a T
  /// @{
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_object(T &&t) noexcept(
      detail::nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                                    decltype(t)>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
  }

  /// in place constructing a T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_object, std::decay_t<T>>>>
  constexpr basic_object(traits::Id<T>, Args &&...args) noexcept(
      detail::nothrow_emplaceable_v<StorageType, T, decltype(args)...>) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
    this->set_vtable(traits::Id<T>{});
    this->set_ptable(traits::Id<T>{});
  }
  /// @}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, const OtherStorage &>>>
  constexpr basic_object &operator=(
      const basic_object<OtherStorage, PropertySpecs, MethodSpecs>
          &other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                        const OtherStorage &>) {
    storage_ = other.storage_;
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, OtherStorage &&>>>
  constexpr basic_object &
  operator=(basic_object<OtherStorage, PropertySpecs, MethodSpecs> &&
                other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                             OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_object, std::decay_t<T>>>>
  constexpr basic_object &
  operator=(T &&t) noexcept(detail::nothrow_emplaceable_v<
                            StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); }
  constexpr const void *data() const noexcept { return storage_.data(); }
  StorageType storage_{};
};

/// specialization for empty PropertySpec list
template <POLY_STORAGE StorageType, template <typename...> typename List,
          POLY_TYPE_LIST MethodSpecs>
class basic_object<StorageType, List<>, MethodSpecs>
    : public detail::MethodContainer<
          basic_object<StorageType, List<>, MethodSpecs>, MethodSpecs> {
public:
  using P =
      detail::PropertyContainer<basic_object<StorageType, List<>, MethodSpecs>,
                                List<>>;
  using M =
      detail::MethodContainer<basic_object<StorageType, List<>, MethodSpecs>,
                              MethodSpecs>;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::MethodContainerImpl;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::PropertyContainer;
  template <POLY_STORAGE S, POLY_TYPE_LIST P, POLY_TYPE_LIST M>
  friend class basic_interface;

  using method_specs = MethodSpecs;
  using property_specs = List<>;

  static_assert(
      poly::is_type_list_v<MethodSpecs>,
      "The MethodSpecs must be provided as a poly::type_list of MethodSpec.");
  static_assert(poly::is_storage_v<StorageType>,
                "StorageType must satisfy the Storage concept.");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<MethodSpecs, traits::is_method_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");

  constexpr basic_object() noexcept = default;

  /// copy ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  constexpr basic_object(
      const basic_object<OtherStorage, List<>, MethodSpecs> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}
  /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
  /// storage type)
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &>>>
  constexpr basic_object(
      basic_object<OtherStorage, List<>, MethodSpecs> &other) noexcept
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// move ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &&>>>
  constexpr basic_object(
      basic_object<OtherStorage, List<>, MethodSpecs>
          &&other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  constexpr basic_object(basic_object &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, StorageType &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}

  /// construct from a T
  /// @{
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_object(T &&t) noexcept(
      detail::nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                                    decltype(t)>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
  }

  /// in place constructing a T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_object, std::decay_t<T>>>>
  constexpr basic_object(traits::Id<T>, Args &&...args) noexcept(
      detail::nothrow_emplaceable_v<StorageType, T, decltype(args)...>) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
    this->set_vtable(traits::Id<T>{});
    this->set_ptable(traits::Id<T>{});
  }
  /// @}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, const OtherStorage &>>>
  constexpr basic_object &operator=(
      const basic_object<OtherStorage, List<>, MethodSpecs>
          &other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                        const OtherStorage &>) {
    storage_ = other.storage_;
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, OtherStorage &&>>>
  constexpr basic_object &
  operator=(basic_object<OtherStorage, List<>, MethodSpecs> &&other) noexcept(
      std::is_nothrow_assignable_v<StorageType, OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_object, std::decay_t<T>>>>
  constexpr basic_object &
  operator=(T &&t) noexcept(detail::nothrow_emplaceable_v<
                            StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); }
  constexpr const void *data() const noexcept { return storage_.data(); }
  StorageType storage_{};
};

template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          template<typename...>typename List>
class basic_object<StorageType,PropertySpecs,List<>>
    : public detail::PropertyContainer<
          basic_object<StorageType, PropertySpecs, List<>>, PropertySpecs> {
public:
  using P = detail::PropertyContainer<
      basic_object<StorageType, PropertySpecs, List<>>, PropertySpecs>;
  using M = detail::MethodContainer<
      basic_object<StorageType, PropertySpecs, List<>>, List<>>;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::MethodContainerImpl;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::PropertyContainer;
  template <POLY_STORAGE S, POLY_TYPE_LIST P, POLY_TYPE_LIST M>
  friend class basic_interface;

  using method_specs = List<>;
  using property_specs = PropertySpecs;

  static_assert(poly::is_type_list_v<PropertySpecs>,
                "The PropertySpecs must be provided as a poly::type_list of "
                "PropertySpec.");
  static_assert(poly::is_storage_v<StorageType>,
                "StorageType must satisfy the Storage concept.");
  static_assert(
      poly::conjunction_v<
          poly::transform_t<PropertySpecs, traits::is_property_spec>>,
      "The provided MethodSpecs must be valid MethodSpecs, i.e. a function "
      "type with signature Ret(MethodName, Args...)[const noexcept].");

  constexpr basic_object() noexcept = default;

  /// copy ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  constexpr basic_object(
      const basic_object<OtherStorage, PropertySpecs, List<>> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}
  /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
  /// storage type)
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &>>>
  constexpr basic_object(
      basic_object<OtherStorage, PropertySpecs, List<>> &other) noexcept
      : storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// move ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &&>>>
  constexpr basic_object(
      basic_object<OtherStorage, PropertySpecs, List<>>
          &&other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  constexpr basic_object(basic_object &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, StorageType &&>)
      : storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}

  /// construct from a T
  /// @{
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_object(T &&t) noexcept(
      detail::nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                                    decltype(t)>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
  }

  /// in place constructing a T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_object, std::decay_t<T>>>>
  constexpr basic_object(traits::Id<T>, Args &&...args) noexcept(
      detail::nothrow_emplaceable_v<StorageType, T, decltype(args)...>) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
    this->set_vtable(traits::Id<T>{});
    this->set_ptable(traits::Id<T>{});
  }
  /// @}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, const OtherStorage &>>>
  constexpr basic_object &operator=(
      const basic_object<OtherStorage, PropertySpecs, List<>>
          &other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                        const OtherStorage &>) {
    storage_ = other.storage_;
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, OtherStorage &&>>>
  constexpr basic_object &
  operator=(basic_object<OtherStorage, PropertySpecs, List<>> &&
                other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                             OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_object, std::decay_t<T>>>>
  constexpr basic_object &
  operator=(T &&t) noexcept(detail::nothrow_emplaceable_v<
                            StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); }
  constexpr const void *data() const noexcept { return storage_.data(); }
  StorageType storage_{};
};
/// @}

/// A Reference is a non owning @ref basic_object "Object" and cheap to copy.
///
/// References can only be created and assigned to from types implementing the
/// properties and methods specified by the PropertySpecs and MethodSpecs, and
/// from other @ref basic_object "Objects" and References with IDENTICAL
/// PropertySpecs and MethodSpecs. References cannot be created from Interfaces.
///
/// @warning  References do not own the bound object, just like normal C++
/// references. Bound objects must be alive for as long as methods and
/// properties of the Reference are accessed.
///
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
using Reference = basic_object<ref_storage, PropertySpecs, MethodSpecs>;

/// @}

} // namespace poly
#endif
