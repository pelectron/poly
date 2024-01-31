#ifndef POLY_INTERFACE_TABLE_HPP
#define POLY_INTERFACE_TABLE_HPP

#include "poly/object_method.hpp"

namespace poly::detail {
/// an entry in the interface vtbale
template <POLY_METHOD_SPEC Spec> struct InterfaceVTableEntry;

template <typename Ret, typename Method, typename... Args>
struct InterfaceVTableEntry<Ret(Method, Args...)> {
  using signature_type = Ret(Method, Args...);
  template <typename... M>
  InterfaceVTableEntry(const VTable<M...> *)
      : offset(VTable<M...>::offset(traits::Id<signature_type>{})) {}
  InterfaceVTableEntry(const InterfaceVTableEntry &) noexcept = default;
  constexpr InterfaceVTableEntry(InterfaceVTableEntry &&) noexcept = default;

  Ret operator()(Method, const void *table, void *obj, Args... args) const {
    assert(table);
    assert(obj);
    const auto *entry = reinterpret_cast<const VTableEntry<signature_type> *>(
        static_cast<const std::byte *>(table) + offset);
    return (*entry)(Method{}, obj, std::forward<Args>(args)...);
  }

  method_offset_type offset{0};
};
template <typename Ret, typename Method, typename... Args>
struct InterfaceVTableEntry<Ret(Method, Args...) const> {
  using signature_type = Ret(Method, Args...);
  template <typename... M>
  InterfaceVTableEntry(const VTable<M...> *)
      : offset(VTable<M...>::offset(traits::Id<signature_type>{})) {}
  constexpr InterfaceVTableEntry(const InterfaceVTableEntry &) noexcept =
      default;
  constexpr InterfaceVTableEntry(InterfaceVTableEntry &&) noexcept = default;

  Ret operator()(Method, const void *table, const void *obj,
                 Args... args) const {
    assert(table);
    assert(obj);
    const auto *entry = reinterpret_cast<const VTableEntry<signature_type> *>(
        static_cast<const std::byte *>(table) + offset);
    return (*entry)(Method{}, obj, std::forward<Args>(args)...);
  }

  method_offset_type offset{0};
};
template <typename Ret, typename Method, typename... Args>
struct InterfaceVTableEntry<Ret(Method, Args...) noexcept> {
  using signature_type = Ret(Method, Args...);
  template <typename... M>
  InterfaceVTableEntry(const VTable<M...> *)
      : offset(VTable<M...>::offset(traits::Id<signature_type>{})) {}
  constexpr InterfaceVTableEntry(const InterfaceVTableEntry &) noexcept =
      default;
  constexpr InterfaceVTableEntry(InterfaceVTableEntry &&) noexcept = default;

  Ret operator()(Method, const void *table, void *obj, Args... args) const {
    assert(table);
    assert(obj);
    const auto *entry = reinterpret_cast<const VTableEntry<signature_type> *>(
        static_cast<const std::byte *>(table) + offset);
    return (*entry)(Method{}, obj, std::forward<Args>(args)...);
  }

  method_offset_type offset{0};
};
template <typename Ret, typename Method, typename... Args>
struct InterfaceVTableEntry<Ret(Method, Args...) const noexcept> {
  using signature_type = Ret(Method, Args...);
  template <typename... M>
  InterfaceVTableEntry(const VTable<M...> *) noexcept
      : offset(VTable<M...>::offset(traits::Id<signature_type>{})) {}
  constexpr InterfaceVTableEntry(const InterfaceVTableEntry &) noexcept =
      default;
  constexpr InterfaceVTableEntry(InterfaceVTableEntry &&) noexcept = default;

  Ret operator()(Method, const void *table, const void *obj,
                 Args... args) const {
    assert(table);
    assert(obj);
    const auto *entry = reinterpret_cast<const VTableEntry<signature_type> *>(
        static_cast<const std::byte *>(table) + offset);
    return (*entry)(Method{}, obj, std::forward<Args>(args)...);
  }

  method_offset_type offset{0};
};

template <POLY_METHOD_SPEC... MethodSpecs>
class InterfaceVTable : private InterfaceVTableEntry<MethodSpecs>... {
  const void *vtable_;
  using InterfaceVTableEntry<MethodSpecs>::operator()...;

public:
  template <typename MethodName, typename... Args>
  static constexpr bool nothrow_callable = noexcept(
      (*std::declval<const detail::InterfaceVTable<MethodSpecs...> *>())(
          MethodName{}, std::declval<const void *>(), std::declval<void *>(),
          std::declval<Args>()...));

  template <typename... M>
  InterfaceVTable(const VTable<M...> *table) noexcept
      : InterfaceVTableEntry<MethodSpecs>(table)..., vtable_(table) {}

  template <typename... M>
  constexpr InterfaceVTable(const InterfaceVTable<M...> &other) noexcept
      : InterfaceVTableEntry<MethodSpecs>(other)..., vtable_(other.vtable_) {}
  constexpr InterfaceVTable(InterfaceVTable &&) noexcept = default;
  constexpr InterfaceVTable(const InterfaceVTable &) noexcept = default;
  constexpr InterfaceVTable &operator=(InterfaceVTable &&) noexcept = default;
  constexpr InterfaceVTable &
  operator=(const InterfaceVTable &) noexcept = default;
  template <typename... M>
  constexpr InterfaceVTable &
  operator=(const InterfaceVTable<M...> &other) noexcept {
    return *this = InterfaceVTable<MethodSpecs...>(other);
  }

  template <typename... M>
  constexpr InterfaceVTable &operator=(InterfaceVTable<M...> &&other) noexcept {
    return *this = std::move(InterfaceVTable<MethodSpecs...>(other));
  }

  template <typename Method, typename... Args>
  decltype(auto) call(Method, void *t, Args &&...args) const
      noexcept(nothrow_callable<Method, Args &&...>) {
    assert(vtable_);
    assert(t);
    return (*this)(Method{}, vtable_, t, std::forward<Args>(args)...);
  }

  template <typename Method, typename... Args>
  decltype(auto) call(Method, const void *t, Args &&...args) const
      noexcept(nothrow_callable<Method, Args &&...>) {
    assert(vtable_);
    assert(t);
    return (*this)(Method{}, vtable_, t, std::forward<Args>(args)...);
  }
};

template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
class InterfaceMethodContainerImpl;

template <typename Self, template <typename...> typename List,
          POLY_METHOD_SPEC... MethodSpecs, typename... CollapsedOverloads>
class InterfaceMethodContainerImpl<Self, List<MethodSpecs...>,
                                   List<CollapsedOverloads...>>
    : public MethodInjector<CollapsedOverloads, Self>... {
public:
  template <typename MethodName, typename... Args>
  static constexpr bool nothrow_callable = noexcept(
      std::declval<const detail::InterfaceVTable<MethodSpecs...>>().call(
          MethodName{},  std::declval<void *>(),
          std::declval<Args>()...));

  template <typename... M>
  InterfaceMethodContainerImpl(const VTable<M...> *vtable) noexcept
      : vtable_(vtable) {
#define POLY_STRINGIFY(x) POLY_STRINGIFY1(x)
#define POLY_STRINGIFY1(x) #x
    static_assert(
        sizeof...(M) <= POLY_MAX_METHOD_COUNT,
        "Error while creating an Interface from an Object: the number of "
        "methods in the Object exceeds POLY_MAX_METHOD_COUNT (=" POLY_STRINGIFY(
            POLY_MAX_METHOD_COUNT) "). Adjust the POLY_MAX_METHOD_COUNT macro "
                                   "to reflect "
                                   "the maximum number of methods accurately "
                                   "before "
                                   "including ANY poly header.");
#undef POLY_STRINGIFY
#undef POLY_STRINGIFY1
  }

  constexpr InterfaceMethodContainerImpl(
      const InterfaceMethodContainerImpl &other) noexcept
      : vtable_(other.vtable_) {}
  constexpr InterfaceMethodContainerImpl(
      InterfaceMethodContainerImpl &&other) noexcept
      : vtable_(std::move(other.vtable_)) {}

  template <typename S, typename LoS, typename CO>
  constexpr InterfaceMethodContainerImpl(
      const InterfaceMethodContainerImpl<S, LoS, CO> &other)
      : vtable_(other.vtable_) {}

  template <typename S, typename LoS, typename CO>
  constexpr InterfaceMethodContainerImpl(
      InterfaceMethodContainerImpl<S, LoS, CO> &&other)
      : vtable_(std::move(other.vtable_)) {}

  template <typename MethodName, typename... Args>
  decltype(auto)
  call(Args &&...args) noexcept(nothrow_callable<MethodName, Args &&...>) {
    return vtable_.call(MethodName{}, self().data(),
                        std::forward<Args>(args)...);
  }

  template <typename MethodName, typename... Args>
  decltype(auto) call(Args &&...args) const
      noexcept(nothrow_callable<MethodName, Args &&...>) {
    return vtable_.call(MethodName{}, self().data(),
                        std::forward<Args>(args)...);
  }

private:
  constexpr Self &self() noexcept { return *static_cast<Self *>(this); }
  constexpr const Self &self() const noexcept {
    return *static_cast<const Self *>(this);
  }
  InterfaceVTable<MethodSpecs...> vtable_;
};

template <typename Self, template <typename...> typename List>
class InterfaceMethodContainerImpl<Self, List<>, List<>> {
  constexpr InterfaceMethodContainerImpl(const void *) noexcept {}
};

template <typename Self, POLY_TYPE_LIST ListOfMethodSpecs>
using InterfaceMethodContainer = InterfaceMethodContainerImpl<
    Self, ListOfMethodSpecs,
    typename collapse_overloads<ListOfMethodSpecs>::type>;

} // namespace poly::detail
#endif
