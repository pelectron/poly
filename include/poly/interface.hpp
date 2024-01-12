#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP
#include "poly/ptable.hpp"
#include "poly/storage.hpp"
#include "poly/vtable.hpp"

namespace poly {

template <POLY_STORAGE StorageType, typename PropertySpecs,
          typename MethodSpecs>
class basic_interface;

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment = alignof(std::max_align_t)>
class Object;

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment = alignof(std::max_align_t)>
class MoveOnlyObject;

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment = alignof(std::max_align_t)>
class SboObject;

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment = alignof(std::max_align_t)>
class SboMoveOnlyObject;

template <typename PropertySpecs, typename MethodSpecs> class Interface;

template <POLY_STORAGE StorageType, typename... PropertySpecs,
          typename... MethodSpecs>
class basic_interface<StorageType, traits::type_list<PropertySpecs...>,
                      traits::type_list<MethodSpecs...>>
    : public detail::PropertyInjector<
          PropertySpecs,
          basic_interface<StorageType, traits::type_list<PropertySpecs...>,
                          traits::type_list<MethodSpecs...>>>...,
      public detail::MethodInjector<
          MethodSpecs,
          basic_interface<StorageType, traits::type_list<PropertySpecs...>,
                          traits::type_list<MethodSpecs...>>>... {
public:
  using properties = traits::type_list<PropertySpecs...>;
  using methods = traits::type_list<MethodSpecs...>;
  static_assert(poly::traits::is_storage_v<StorageType>,
                "StorageType must satisfy the Storage concept.");
  static_assert((poly::traits::is_property_spec_v<PropertySpecs> && ...),
                "The provided PropertySpecs must be valid PropertySpecs.");
  static_assert((poly::traits::is_method_spec_v<MethodSpecs> && ...),
                "The provided MethodSpecs must be valid MethodSpecs.");

  basic_interface() = delete;

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  basic_interface(const basic_interface &other)
      : vtable_(other.vtable_), ptable_(other.ptable_),
        storage_(other.storage_) {}

  template <POLY_STORAGE OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<StorageType, OtherStorage>>>
  basic_interface(basic_interface<OtherStorage, properties, methods> &other)
      : vtable_(other.vtable_), ptable_(other.ptable_),
        storage_(other.storage_) {}

  basic_interface(basic_interface &&) = default;

  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  basic_interface(T &&t)
      : vtable_(std::addressof(
            detail::vtable_for<std::decay_t<T>, MethodSpecs...>)),
        ptable_(std::addressof(
            detail::ptable_for<std::decay_t<T>, PropertySpecs...>)),
        storage_(std::forward<T>(t)) {}

  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<basic_interface, std::decay_t<T>>>>
  basic_interface(std::in_place_type_t<T>, Args &&...args)
      : vtable_(std::addressof(detail::vtable_for<T, MethodSpecs...>)),
        ptable_(std::addressof(detail::ptable_for<T, PropertySpecs...>)) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
  }

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  basic_interface &operator=(const basic_interface &other) {
    storage_ = other.storage_;
    vtable_ = other.vtable_;
    ptable_ = other.ptable_;
    return *this;
  }

  basic_interface &operator=(basic_interface &&) = default;

  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_interface, std::decay_t<T>>>>
  basic_interface &operator=(T &&t) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    vtable_ =
        std::addressof(detail::vtable_for<std::decay_t<T>, MethodSpecs...>);
    ptable_ =
        std::addressof(detail::ptable_for<std::decay_t<T>, PropertySpecs...>);
    return *this;
  }
  template <typename T, typename = std::enable_if_t<not std::is_base_of_v<
                            basic_interface, std::decay_t<T>>>>
  basic_interface &operator=(const T &t) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    vtable_ =
        std::addressof(detail::vtable_for<std::decay_t<T>, MethodSpecs...>);
    ptable_ =
        std::addressof(detail::ptable_for<std::decay_t<T>, PropertySpecs...>);
    return *this;
  }

  template <typename Method, typename... Args>
  decltype(auto) call(Args &&...args) {
    return (*vtable_)(Method{}, storage_.data(), std::forward<Args>(args)...);
  }
  template <typename Method, typename... Args>
  decltype(auto) call(Args &&...args) const {
    return (*vtable_)(Method{}, storage_.data(), std::forward<Args>(args)...);
  }
  template <typename PropertyName> decltype(auto) get() const {
    return ptable_->get(PropertyName{}, storage_.data());
  }
  template <typename PropertyName, typename Type> void set(const Type &value) {
    ptable_->set(PropertyName{}, storage_.data(), value);
  }

protected:
  const detail::VTable<MethodSpecs...> *vtable_;
  const detail::PTable<PropertySpecs...> *ptable_;
  StorageType storage_;
};

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment>
class Object : public basic_interface<local_storage<Size, Alignment>,
                                      PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<local_storage<Size, Alignment>, PropertySpecs,
                               MethodSpecs>;
  using Base::operator=;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  Object(T &&t) : Base(std::forward<T>(t)) {}

  Object(const Object &other) : Base(other) {}

  Object(Object &&) = default;

  Object &operator=(const Object &other) = default;
  Object &operator=(Object &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment>
class MoveOnlyObject
    : public basic_interface<local_move_only_storage<Size, Alignment>,
                             PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<local_move_only_storage<Size, Alignment>,
                               PropertySpecs, MethodSpecs>;
  using Base::operator=;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  MoveOnlyObject(T &&t) : Base(std::forward<T>(t)) {}
  MoveOnlyObject(MoveOnlyObject &&) = default;
  MoveOnlyObject(const MoveOnlyObject &other) = delete;
  MoveOnlyObject &operator=(const MoveOnlyObject &other) = delete;
  MoveOnlyObject &operator=(MoveOnlyObject &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment>
class SboObject : public basic_interface<sbo_storage<Size, Alignment>,
                                         PropertySpecs, MethodSpecs> {
public:
  using Base =
      basic_interface<sbo_storage<Size, Alignment>, PropertySpecs, MethodSpecs>;
  using Base::operator=;
  template <typename T, typename = std::enable_if_t<
                            not traits::is_storage_v<std::decay_t<T>>>>
  SboObject(T &&t) : Base(std::forward<T>(t)) {}
  SboObject(const SboObject &other) : Base(other) {}
  SboObject(SboObject &&) = default;
  SboObject &operator=(const SboObject &other) = default;
  SboObject &operator=(SboObject &&) = default;
};

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment>
class SboMoveOnlyObject
    : public basic_interface<sbo_move_only_storage<Size, Alignment>,
                             PropertySpecs, MethodSpecs> {
public:
  using Base = basic_interface<sbo_move_only_storage<Size, Alignment>,
                               PropertySpecs, MethodSpecs>;
  using Base::operator=;
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
