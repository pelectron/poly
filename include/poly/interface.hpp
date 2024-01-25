#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP
#include "poly/config.hpp"

#include "poly/method.hpp"
#include "poly/property.hpp"
#include "poly/storage.hpp"
#include "poly/traits.hpp"
#include <cstddef>

namespace poly {
namespace detail {} // namespace detail
template <typename Storage, typename T, typename... Args>
inline constexpr bool nothrow_emplaceable_v = noexcept(
    std::declval<Storage>().template emplace<T>(std::declval<Args>()...));

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class Object;

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class MoveOnlyObject;

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class SboObject;

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class SboMoveOnlyObject;

/// This class is the replacement for the pointer to base.
/// It holds objects in a storage of type StorageType, and provides the
/// properties and methods given by the list of PropertySpecs and MethodSpecs.
///
/// Objects can only be created and assigned to from types implementing the
/// propertes and methods specified, and from other Objects with IDENTICAL
/// PropertySpecs and MethodSpecs.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// poly::Storage concept. Usually owning.
/// @tparam PropertySpecs a poly::type_list of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a poly::type_list of @ref MethodSpec "MethodSpecs"
template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
class basic_object
    : public detail::PropertyContainer<
          basic_object<StorageType, PropertySpecs, MethodSpecs>, PropertySpecs>,
      public detail::MethodContainer<
          basic_object<StorageType, PropertySpecs, MethodSpecs>, MethodSpecs> {
  using P = detail::PropertyContainer<
      basic_object<StorageType, PropertySpecs, MethodSpecs>, PropertySpecs>;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::MethodContainerImpl;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::PropertyContainer;
  template <typename S, POLY_TYPE_LIST P, POLY_TYPE_LIST M> friend class basic_interface;

public:
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

  constexpr basic_object() noexcept : P(*this) {}

  /// copy ctor
  /// @{
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  constexpr basic_object(
      const basic_object<OtherStorage, PropertySpecs, MethodSpecs> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : P(*this), storage_(other.storage_) {
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
      : P(*this), storage_(other.storage_) {
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
      : P(*this), storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  constexpr basic_object(basic_object &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, StorageType &&>)
      : P(*this), storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }
  /// @}

  /// construct from a T
  /// @{
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  constexpr basic_object(T &&t) noexcept(
      nothrow_emplaceable_v<StorageType, std::decay_t<T>, decltype(t)>)
      : P(*this) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
  }

  /// in place constructing a T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_object, std::decay_t<T>>>>
  constexpr basic_object(traits::Id<T>, Args &&...args) noexcept(
      nothrow_emplaceable_v<StorageType, T, decltype(args)...>)
      : P(*this) {
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
  constexpr basic_object &operator=(T &&t) noexcept(
      nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); }
  constexpr const void *data() const noexcept { return storage_.data(); }
  StorageType storage_;
};

template <POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
using Reference = basic_object<ref_storage, PropertySpecs, MethodSpecs>;

/// An interface is a subset of an object, i.e. an interface has a reduced
/// number of properties and methods.
template <typename StorageType, POLY_TYPE_LIST PropertySpecs, POLY_TYPE_LIST MethodSpecs>
class basic_interface;
template <typename StorageType, typename... PropertySpecs,
          typename... MethodSpecs>
class basic_interface<StorageType, type_list<PropertySpecs...>,
                      type_list<MethodSpecs...>>
    : public detail::InterfacePropertyContainer<
          basic_interface<StorageType, type_list<PropertySpecs...>,
                          type_list<MethodSpecs...>>,
          type_list<PropertySpecs...>>,
      public detail::InterfaceMethodContainer<
          basic_interface<StorageType, type_list<PropertySpecs...>,
                          type_list<MethodSpecs...>>,
          type_list<MethodSpecs...>> {
  using M = detail::InterfaceMethodContainer<
      basic_interface<StorageType, type_list<PropertySpecs...>,
                      type_list<MethodSpecs...>>,
      type_list<MethodSpecs...>>;
  using P = detail::InterfacePropertyContainer<
      basic_interface<StorageType, type_list<PropertySpecs...>,
                      type_list<MethodSpecs...>>,
      type_list<PropertySpecs...>>;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::InterfacePropertyContainer;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::InterfaceMethodContainerImpl;

public:
  template <typename S, typename Ps, typename Ms>
  constexpr basic_interface(basic_object<S, Ps, Ms> &intf)
      : P(*this, intf.ptable()), M(intf.vtable()), storage_(intf.storage_) {}

  template <typename S, typename Ps, typename Ms,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const S &>>>
  constexpr basic_interface(const basic_object<S, Ps, Ms> &intf) noexcept(
      std::is_nothrow_constructible_v<StorageType, const S &>)
      : P(*this, intf.ptable()), M(intf.vtable()), storage_(intf.storage_) {}

  template <
      typename S, typename Ps, typename Ms,
      typename = std::enable_if_t<std::is_constructible_v<StorageType, S &&>>>
  constexpr basic_interface(basic_object<S, Ps, Ms> &&intf) noexcept(
      std::is_nothrow_constructible_v<StorageType, S &&>)
      : P(*this, intf.ptable()), M(intf.vtable()),
        storage_(std::move(intf.storage_)) {}

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  constexpr basic_interface(const basic_interface &other) noexcept(
      std::is_nothrow_copy_constructible_v<S>)
      : P(std::move(other)), M(std::move(other)),
        storage_(std::move(other.storage_)) {}

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  constexpr basic_interface(basic_interface &&other) noexcept(
      std::is_nothrow_move_constructible_v<S>)
      : P(other), M(other), storage_(other.storage_) {}

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  constexpr basic_interface &operator=(const basic_interface &other) noexcept(
      std::is_nothrow_constructible_v<S>) {
    P::operator=(other);
    M::operator=(other);
    storage_ = other.storage_;
    return *this;
  }

  constexpr basic_interface &operator=(basic_interface &&other) noexcept(
      std::is_nothrow_move_constructible_v<StorageType>) {
    P::operator=(std::move(other));
    M::operator=(std::move(other));
    storage_ = std::move(other.storage_);
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); };
  constexpr const void *data() const noexcept { return storage_.data(); };

  StorageType storage_;
};

template <typename PropertySpecs, typename MethodSpecs>
class Interface
    : public basic_interface<ref_storage, PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<ref_storage, PropertySpecs, MethodSpecs>;
  using Base::Base;

  constexpr Interface(const Interface &other) noexcept : Base(other) {}
  constexpr Interface(Interface &&other) noexcept : Base(std::move(other)) {}

  template <POLY_STORAGE StorageType>
  constexpr Interface(
      basic_object<StorageType, PropertySpecs, MethodSpecs> &object)
      : Base(object) {}

  constexpr Interface &operator=(const Interface &other) noexcept {
    Base::operator=(other);
    return *this;
  }

  constexpr Interface &operator=(Interface &&other) noexcept {
    Base::operator=(std::move(other));
    return *this;
  }
};
} // namespace poly
#endif
