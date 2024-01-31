#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP

#include "poly/interface_method.hpp"
#include "poly/interface_property.hpp"
#include "poly/object.hpp"

namespace poly {

/// @addtogroup interface Interface
/// An Interface provides a subset of an Object.
/// @{

/// An Interface provides a subset of an Object. They can constrain existing
/// Object types and are more flexible, as they can be created from any Object
/// or Interface providing a super set of the methods and properties specified
/// by the MethodSpecs and PropertySpecs.
///
/// Interfaces can only be created from Objects/Interfaces providing a super set
/// of properties and methods, and not directly from types implementing the
/// specified methods and properties.
///
/// @note Interfaces do not generate extra vtables and property tables, but
/// adapt the tables from the Object the Interface is created from. This
/// necessitates extra stack space for each instance, and disables constexpr use
/// because of pointer arithmetic and casting.
///
/// @warning If an Object with more than 255 properties is bound to an Interface
/// with at least one property, the macro POLY_MAX_PROPERTY_COUNT must be set
/// accordingly before including any poly header.
///
/// @warning If an Object with more than 255 methods is bound to an Interface
/// with at least one method, the macro POLY_MAX_METHOD_COUNT must be set
/// accordingly before including any poly header.
///
/// @tparam StorageType storage used for objects emplaced. Must conform to the
/// @ref Storage "poly::Storage" concept.
/// @tparam PropertySpecs a TypeList of @ref PropertySpec "PropertySpecs"
/// @tparam MethodSpecs a TypeList of @ref MethodSpec "MethodSpecs"
/// @{
template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
class basic_interface;

template <POLY_STORAGE StorageType, template <typename...> typename List,
          typename... PropertySpecs, typename... MethodSpecs>
class POLY_EMPTY_BASE basic_interface<StorageType, List<PropertySpecs...>, List<MethodSpecs...>>
    : public detail::InterfacePropertyContainer<
          basic_interface<StorageType, List<PropertySpecs...>,
                          List<MethodSpecs...>>,
          List<PropertySpecs...>>,
      public detail::InterfaceMethodContainer<
          basic_interface<StorageType, List<PropertySpecs...>,
                          List<MethodSpecs...>>,
          List<MethodSpecs...>> {
  using M = detail::InterfaceMethodContainer<
      basic_interface<StorageType, List<PropertySpecs...>,
                      List<MethodSpecs...>>,
      List<MethodSpecs...>>;
  using P = detail::InterfacePropertyContainer<
      basic_interface<StorageType, List<PropertySpecs...>,
                      List<MethodSpecs...>>,
      List<PropertySpecs...>>;
  template <typename Self, POLY_TYPE_LIST ListOfPropertySpecs>
  friend class poly::detail::InterfacePropertyContainer;

  template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
  friend class poly::detail::InterfaceMethodContainerImpl;
  template <POLY_STORAGE S, POLY_TYPE_LIST Ps, POLY_TYPE_LIST Ms>
  friend class basic_interface;

public:
  template <typename S, typename Ps, typename Ms>
  basic_interface(basic_object<S, Ps, Ms> &intf)
      : P(intf.ptable()), M(intf.vtable()), storage_(intf.storage_) {}

  template <typename S, typename Ps, typename Ms,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const S &>>>
  basic_interface(const basic_object<S, Ps, Ms> &intf) noexcept(
      std::is_nothrow_constructible_v<StorageType, const S &>)
      : P(intf.ptable()), M(intf.vtable()), storage_(intf.storage_) {}

  template <
      typename S, typename Ps, typename Ms,
      typename = std::enable_if_t<std::is_constructible_v<StorageType, S &&>>>
  basic_interface(basic_object<S, Ps, Ms> &&intf) noexcept(
      std::is_nothrow_constructible_v<StorageType, S &&>)
      : P(intf.ptable()), M(intf.vtable()), storage_(std::move(intf.storage_)) {
  }

  basic_interface(const basic_interface &other) noexcept(
      std::is_nothrow_copy_constructible_v<StorageType>)
      : P(other), M(other), storage_(other.storage_) {}

  basic_interface(basic_interface &&other) noexcept(
      std::is_nothrow_move_constructible_v<StorageType>)
      : P(other), M(other), storage_(std::move(other.storage_)) {}

  template <typename S, typename Ps, typename Ms,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, const S &>>>
  basic_interface(const basic_interface<S, Ps, Ms> &other) noexcept(
      std::is_nothrow_constructible_v<StorageType, const S &>)
      : P(other), M(other), storage_(other.storage_) {}

  template <
      typename S, typename Ps, typename Ms,
      typename = std::enable_if_t<std::is_constructible_v<StorageType, S &&>>>
  basic_interface(basic_interface<S, Ps, Ms> &&other) noexcept(
      std::is_nothrow_constructible_v<StorageType, S &&>)
      : P(other), M(other), storage_(std::move(other.storage_)) {}

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  basic_interface &operator=(const basic_interface &other) noexcept(
      std::is_nothrow_constructible_v<S>) {
    storage_ = other.storage_;
    P::operator=(other);
    M::operator=(other);
    return *this;
  }

  basic_interface &operator=(basic_interface &&other) noexcept(
      std::is_nothrow_move_constructible_v<StorageType>) {
    storage_ = std::move(other.storage_);
    P::operator=(other);
    M::operator=(other);
    return *this;
  }
  template <
      typename S, typename Ps, typename Ms,
      typename = std::enable_if_t<std::is_assignable_v<StorageType, S &&>>>
  basic_interface &operator=(basic_interface<S, Ps, Ms> &&other) noexcept(
      std::is_nothrow_assignable_v<StorageType, S &&>) {
    storage_ = std::move(other.storage_);
    P::operator=(other);
    M::operator=(other);
    return *this;
  }
  template <
      typename S, typename Ps, typename Ms,
      typename = std::enable_if_t<std::is_assignable_v<StorageType, const S &>>>
  basic_interface &operator=(const basic_interface<S, Ps, Ms> &other) noexcept(
      std::is_nothrow_assignable_v<StorageType, const S &>) {
    storage_ = other.storage_;
    P::operator=(other);
    M::operator=(other);
    return *this;
  }

private:
  constexpr void *data() noexcept { return storage_.data(); };
  constexpr const void *data() const noexcept { return storage_.data(); };

  StorageType storage_;
};
/// @}

/// An non owning Interface. Can be used to pass Objects, References and other
/// Interfaces.
///
/// @warning The same lifetime restrictions apply as with @ref Reference
/// "References".
template <typename PropertySpecs, typename MethodSpecs>
using Interface = basic_interface<ref_storage, PropertySpecs, MethodSpecs>;

/// @}
} // namespace poly
#endif
