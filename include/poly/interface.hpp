#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP
#include "poly/config.hpp"

#include "poly/method.hpp"
#include "poly/traits.hpp"
#include "poly/property.hpp"
#include "poly/storage.hpp"
#include <cstddef>

namespace poly {
namespace detail {} // namespace detail
template <typename Storage, typename T, typename... Args>
inline constexpr bool nothrow_emplaceable_v = noexcept(
    std::declval<Storage>().template emplace<T>(std::declval<Args>()...));

template <POLY_STORAGE StorageType, typename PropertySpecs,
          typename MethodSpecs>
class basic_interface;

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class Object;

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class MoveOnlyObject;

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class SboObject;

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment = alignof(std::max_align_t)>
class SboMoveOnlyObject;

template <typename PropertySpecs, typename MethodSpecs> class Interface;

template <POLY_STORAGE StorageType, typename PropertySpecs,
          typename MethodSpecs>
class basic_interface
    : public detail::PropertyContainer<
          basic_interface<StorageType, PropertySpecs, MethodSpecs>,
          PropertySpecs>,
      public detail::MethodContainer<
          basic_interface<StorageType, PropertySpecs, MethodSpecs>,
          MethodSpecs> {
  using P = detail::PropertyContainer<
      basic_interface<StorageType, PropertySpecs, MethodSpecs>, PropertySpecs>;

public:
  template <typename Self, typename ListOfSpecs, typename>
  friend class poly::detail::MethodContainerImpl;
  template <typename Self, typename ListOfPropertySpecs>
  friend class poly::detail::PropertyContainer;

  using method_specs = MethodSpecs;
  using property_specs =PropertySpecs;

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
  basic_interface() = delete;

  /// copy ctor
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const OtherStorage &>>>
  basic_interface(
      const basic_interface<OtherStorage, PropertySpecs, MethodSpecs> &
          other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                          const OtherStorage &>)
      : P(*this), storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// ctor for lvalue reference (Storage = ref storage, OtherStorage= any
  /// storage type)
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &>>>
  basic_interface(
      basic_interface<OtherStorage, PropertySpecs, MethodSpecs> &other) noexcept
      : P(*this), storage_(other.storage_) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// move ctor
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage &&>>>
  basic_interface(
      basic_interface<OtherStorage, PropertySpecs, MethodSpecs>
          &&other) noexcept(std::is_nothrow_constructible_v<StorageType,
                                                            OtherStorage &&>)
      : P(*this), storage_(std::move(other.storage_)) {
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
  }

  /// construct from a T
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  basic_interface(T &&t) noexcept(
      nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>)
      : P(*this) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
  }

  /// construct from T by in place constructing T
  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_interface, std::decay_t<T>>>>
  basic_interface(traits::Id<T>, Args &&...args) noexcept(
      nothrow_emplaceable_v<
          StorageType, std::decay_t<T>,
          decltype(std::forward<Args>(std::declval<Args &&>()))...>)
      : P(*this) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
    this->set_vtable(traits::Id<T>{});
    this->set_ptable(traits::Id<T>{});
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<StorageType, const OtherStorage &>>>
  basic_interface &operator=(
      const basic_interface<OtherStorage, PropertySpecs, MethodSpecs>
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
  basic_interface &
  operator=(basic_interface<OtherStorage, PropertySpecs, MethodSpecs> &&
                other) noexcept(std::is_nothrow_assignable_v<StorageType,
                                                             OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    this->set_vtable(other.vtable());
    this->set_ptable(other.ptable());
    return *this;
  }

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_interface, std::decay_t<T>>>>
  basic_interface &operator=(T &&t) noexcept(
      nothrow_emplaceable_v<StorageType, std::decay_t<T>,
                            decltype(std::forward<T>(std::declval<T &&>()))>) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    this->set_vtable(traits::Id<std::decay_t<T>>{});
    this->set_ptable(traits::Id<std::decay_t<T>>{});
    return *this;
  }

private:
  void *data() noexcept { return storage_.data(); }
  const void *data() const noexcept { return storage_.data(); }
  StorageType storage_;
};

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment>
class Object : public basic_interface<local_storage<Size, Alignment>,
                                      PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<local_storage<Size, Alignment>, PropertySpecs,
                               MethodSpecs>;
  using self_type = Object<PropertySpecs, MethodSpecs, Size, Alignment>;
  using Base::operator=;
  using Base::call;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  Object(T &&t) : Base(std::forward<T>(t)) {}

  Object(const Object &other) : Base(other) {}

  Object(Object &&) = default;

  Object &operator=(const Object &other) = default;
  Object &operator=(Object &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment>
class MoveOnlyObject
    : public basic_interface<move_only_local_storage<Size, Alignment>,
                             PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<move_only_local_storage<Size, Alignment>,
                               PropertySpecs, MethodSpecs>;
  using Base::operator=;
  using Base::call;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  MoveOnlyObject(T &&t) : Base(std::forward<T>(t)) {}
  MoveOnlyObject(MoveOnlyObject &&) = default;
  MoveOnlyObject(const MoveOnlyObject &other) = delete;
  MoveOnlyObject &operator=(const MoveOnlyObject &other) = delete;
  MoveOnlyObject &operator=(MoveOnlyObject &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment>
class SboObject : public basic_interface<sbo_storage<Size, Alignment>,
                                         PropertySpecs, MethodSpecs> {
public:
  using Base =
      basic_interface<sbo_storage<Size, Alignment>, PropertySpecs, MethodSpecs>;
  using Base::operator=;
  using Base::call;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  SboObject(T &&t) : Base(std::forward<T>(t)) {}
  SboObject(const SboObject &other) : Base(other) {}
  SboObject(SboObject &&) = default;
  SboObject &operator=(const SboObject &other) = default;
  SboObject &operator=(SboObject &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, std::size_t Size,
          std::size_t Alignment>
class SboMoveOnlyObject
    : public basic_interface<move_only_sbo_storage<Size, Alignment>,
                             PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<move_only_sbo_storage<Size, Alignment>,
                               PropertySpecs, MethodSpecs>;
  using Base::operator=;
  using Base::call;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  SboMoveOnlyObject(T &&t) : Base(std::forward<T>(t)) {}
  SboMoveOnlyObject(SboMoveOnlyObject &&) = default;
  SboMoveOnlyObject(const SboMoveOnlyObject &other) = delete;
  SboMoveOnlyObject &operator=(const SboMoveOnlyObject &other) = delete;
  SboMoveOnlyObject &operator=(SboMoveOnlyObject &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs>
class Interface
    : public basic_interface<ref_storage, PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<ref_storage, PropertySpecs, MethodSpecs>;
  using Base::operator=;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  Interface(T &&t) : Base(std::forward<T>(t)) {}
  Interface(const Interface &other) : Base(other) {}
  Interface(Interface &&) = default;
  template <POLY_STORAGE StorageType>
  Interface(basic_interface<StorageType, PropertySpecs, MethodSpecs> &object)
      : Base(object) {}
  Interface &operator=(const Interface &other) = default;
  Interface &operator=(Interface &&) = default;
};

} // namespace poly
#endif
