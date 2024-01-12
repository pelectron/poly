#ifndef POLY_INTERFACE_HPP
#define POLY_INTERFACE_HPP
#include "poly/ptable.hpp"
#include "poly/storage.hpp"
#include "poly/vtable.hpp"

namespace poly {

template <POLY_STORAGE StorageType, typename PropertySpecs,
          typename MethodSpecs>
class Interface;

template <POLY_STORAGE StorageType, typename... PropertySpecs,
          typename... MethodSpecs>
class Interface<StorageType, traits::type_list<PropertySpecs...>,
                traits::type_list<MethodSpecs...>>
    : public detail::PropertyInjector<
          PropertySpecs,
          Interface<StorageType, traits::type_list<PropertySpecs...>,
                    traits::type_list<MethodSpecs...>>>...,
      public detail::MethodInjector<
          MethodSpecs,
          Interface<StorageType, traits::type_list<PropertySpecs...>,
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

  Interface() = delete;

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  Interface(const Interface &other)
      : vtable_(other.vtable_), ptable_(other.ptable_),
        storage_(other.storage_) {}

  template <POLY_STORAGE OtherStorage>
  Interface(Interface<OtherStorage, properties, methods> &other)
      : vtable_(other.vtable_), ptable_(other.ptable_),
        storage_(other.storage_) {}

  Interface(Interface &&) = default;

  template <typename T, typename = std::enable_if_t<
                            not std::is_base_of_v<Interface, std::decay_t<T>>>>
  Interface(T &&t)
      : vtable_(std::addressof(
            detail::vtable_for<std::decay_t<T>, MethodSpecs...>)),
        ptable_(std::addressof(
            detail::ptable_for<std::decay_t<T>, PropertySpecs...>)),
        storage_(std::forward<T>(t)) {}

  template <typename T, typename... Args,
            typename = std::enable_if_t<
                not std::is_base_of_v<Interface, std::decay_t<T>>>>
  Interface(std::in_place_type_t<T>, Args &&...args)
      : vtable_(std::addressof(detail::vtable_for<T, MethodSpecs...>)),
        ptable_(std::addressof(detail::ptable_for<T, PropertySpecs...>)) {
    storage_.template emplace<T>(std::forward<Args>(args)...);
  }

  template <typename S = StorageType,
            typename = std::enable_if_t<std::is_copy_constructible_v<S>>>
  Interface &operator=(const Interface &other) {
    storage_ = other.storage_;
    vtable_ = other.vtable_;
    ptable_ = other.ptable_;
    return *this;
  }

  Interface &operator=(Interface &&) = default;

  template <typename T, typename = std::enable_if_t<
                            not std::is_base_of_v<Interface, std::decay_t<T>>>>
  Interface &operator=(T &&t) {
    storage_.template emplace<std::decay_t<T>>(std::forward<T>(t));
    vtable_ =
        std::addressof(detail::vtable_for<std::decay_t<T>, MethodSpecs...>);
    ptable_ =
        std::addressof(detail::ptable_for<std::decay_t<T>, PropertySpecs...>);
    return *this;
  }
  template <typename T, typename = std::enable_if_t<
                            not std::is_base_of_v<Interface, std::decay_t<T>>>>
  Interface &operator=(const T &t) {
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

private:
  const detail::VTable<MethodSpecs...> *vtable_;
  const detail::PTable<PropertySpecs...> *ptable_;
  StorageType storage_;
};

template <typename PropertySpecs, typename MethodSpecs, size_t Size,
          size_t Alignment = alignof(std::max_align_t), bool Copyable = false>
class Object : public InterfaceImpl<local_storage<Copyable, Size, Alignment>,
                                    PropertySpecs, MethodSpecs> {
public:
  using Base = InterfaceImpl<local_storage<Copyable, Size, Alignment>,
                             PropertySpecs, MethodSpecs>;
  template <typename T,
            typename = std::enable_if_t<not std::is_same_v<T, Object>>>
  Object(const T &t) : Base(t) {}

  template <bool Enable = Copyable, std::enable_if_t<Enable>>
  Object(const Object &other) : Base(other) {}

  Object(Object &&) = default;

  template <bool Enable = Copyable, std::enable_if_t<Enable>>
  Object &operator=(const Object &other) {
    *static_cast<Base *>(this) = static_cast<const Base &>(other);
    return *this;
  }
  Object &operator=(Object &&) = default;
  template <typename T, typename = std::enable_if_t<
                            not std::is_same_v<Object, std::decay_t<T>>>>
  Object &operator=(T &&t) {
    *static_cast<Base *>(this) = std::forward<T>(t);
    return *this;
  }
};

} // namespace poly
#endif
