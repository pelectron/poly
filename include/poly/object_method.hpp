#ifndef POLY_OBJECT_TABLE_HPP
#define POLY_OBJECT_TABLE_HPP
#include "poly/method.hpp"
#include "poly/traits.hpp"

#include <cassert>
namespace poly::detail{

/// The MethodInjector is used to inject methods by name.
/// @{
/// default method injector does nothing
template <typename MethodSpecOrListOfSpecs, typename Self, typename = void>
struct MethodInjector {};

/// if the method was created with the POLY_METHOD macro, there will be an
/// innner template named "injector" present in the method, which is templated
/// on a type Self and the MethodSpec. The injector will link the Method with
/// its real name, i.e. "Method". This allows the end user to use the syntax
/// obj.Method(args...) instead of obj.call<Method>(args...).
/// @{
template <POLY_METHOD_SPEC Spec, typename Self>
struct MethodInjector<
    Spec, Self,
    std::void_t<typename method_name_t<Spec>::template injector<Self, Spec>>>
    : public method_name_t<Spec>::template injector<Self, Spec> {};

/// sepcialization for overloaded methods
template <template <typename...> typename List, POLY_METHOD_SPEC... MethodSpecs,
          typename Self>
struct MethodInjector<
    List<MethodSpecs...>, Self,
    std::void_t<typename poly::method_name_t<at_t<List<MethodSpecs...>, 0>>::
                    template injector<Self, List<MethodSpecs...>>>>
    : public poly::method_name_t<at_t<List<MethodSpecs...>, 0>>::
          template injector<Self, List<MethodSpecs...>> {};
/// @}
/// @}

template <typename Name, POLY_TYPE_LIST SpecList> struct build_method_set;
template <typename Name, template <typename...> typename List,
          POLY_METHOD_SPEC... Specs>
struct build_method_set<Name, List<Specs...>> {
  template <typename T> using predicate = std::is_same<Name, method_name_t<T>>;
  template <typename T> struct neg_predicate {
    static constexpr bool value = not predicate<T>::value;
  };

  using overloaded_specs = filter_t<List<Specs...>, predicate>;
  // spec to add to output. if not an overload, add the overloaded_specs
  // directly to output(unwrapped in build_method_sets), else wrap the overload
  // list in another TypeList.
  using spec = std::conditional_t<list_size<overloaded_specs>::value == 1,
                                  overloaded_specs, List<overloaded_specs>>;
  // the remaining specs which did not match Name
  using output_list = filter_t<List<Specs...>, neg_predicate>;
};

template <POLY_TYPE_LIST NameList, POLY_TYPE_LIST SpecList>
struct build_method_sets;
template <template <typename...> typename List>
struct build_method_sets<List<>, List<>> {
  using type = List<>;
};
template <typename Name, template <typename...> typename List,
          typename... Names, POLY_METHOD_SPEC... Specs>
struct build_method_sets<List<Name, Names...>, List<Specs...>> {
  using bms = build_method_set<Name, List<Specs...>>;
  using rest = build_method_sets<List<Names...>, typename bms::output_list>;
  using type = typename concat<typename bms::spec, typename rest::type>::type;
};
/// turns the flat list of MethodSpecs into a list of non overloaded MethodSpecs
/// and type lists of overloaded MethodSpecs.
///
/// Pseudocode:
/// for non duplicate name in the list of MethodSpecs, append the MethodSpec to
/// the output for each duplicate name:
///    filter the list of MethodSpecs for the MethodSpec with the same name,
///    add them into a list
///    add the list to the output as an element
/// Example:
/// - collapse_overloads<type_list<void(m1),void(m2)>>::type ==
/// type_list<void(m1),void(m2)>
/// - collapse_overloads<type_list<void(m1),void(m2),void(m2,int)>>::type ==
/// type_list<void(m1),type_list<void(m2),void(m2,int)>>
///@{
template <POLY_TYPE_LIST MethodSpecs> struct collapse_overloads;
template <template <typename...> typename List>
struct collapse_overloads<List<>> {
  using type = List<>;
};
template <template <typename...> typename List, POLY_METHOD_SPEC... MethodSpecs>
struct collapse_overloads<List<MethodSpecs...>>
    : build_method_sets<
          typename remove_duplicates<List<method_name_t<MethodSpecs>...>>::type,
          List<MethodSpecs...>> {};

static_assert(
    std::is_same_v<
        type_list<type_list<int(float, float), int(float, int)>, int(int),
                  int(double)>,
        typename collapse_overloads<type_list<
            int(float, float), int(int), int(double), int(float, int)>>::type>);
/// @}

/// This struct provides the static member function template jump(). jump() is
/// the type erased function called by the vtable for the MethdoSpec.
/// @{
template <POLY_METHOD_SPEC MethodSpec> struct trampoline;

template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...)> {
  template <typename T>
  static constexpr Ret jump(Method, void *t, Args... args) {
    using poly::extend;
    return extend(Method{}, *static_cast<T *>(t), std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) const> {
  template <typename T>
  static constexpr Ret jump(Method, const void *t, Args... args) {
    using poly::extend;
    return extend(Method{}, *static_cast<const T *>(t),
                  std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) noexcept> {
  template <typename T>
  static constexpr Ret jump(Method, void *t, Args... args) noexcept {
    static_assert(noexcept(extend(Method{}, *static_cast<T *>(t),
                                  std::forward<Args>(args)...)),
                  "Ret extend(Method, T&, Args...) is not declared noexcept, "
                  "but method is specified noexcept");
    using poly::extend;
    return extend(Method{}, *static_cast<T *>(t), std::forward<Args>(args)...);
  }
};
template <typename Ret, typename Method, typename... Args>
struct trampoline<Ret(Method, Args...) const noexcept> {
  template <typename T>
  static constexpr Ret jump(Method, const void *t, Args... args) noexcept {
    static_assert(
        noexcept(extend(Method{}, *static_cast<const T *>(t),
                        std::forward<Args>(args)...)),
        "Ret extend(Method, const T&, Args...) is not declared noexcept, "
        "but method is specified noexcept");
    using poly::extend;
    return extend(Method{}, *static_cast<const T *>(t),
                  std::forward<Args>(args)...);
  }
};
/// @}

/// @addtogroup vtable Vtable
/// Individual VTable entry. Stores the address of trampoline<MethodSpec>::jump
/// and provides a function call operator to invoke the function.
/// @{
template <typename MethodSpec> struct VTableEntry;

/// VTable specialization for non const method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...)> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...)>::template jump<T>)) {}

  constexpr VTableEntry() noexcept {}

  constexpr Ret operator()(Method m, void *t, Args... args) const {
    assert(t);
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  using func_pointer_t = Ret (*)(Method, void *t, Args...);
  func_pointer_t func{nullptr};
};

/// VTable specialization for const method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr VTableEntry() noexcept {}

  constexpr Ret operator()(Method m, const void *t, Args... args) const {
    assert(t);
    return *func(m, t, std::forward<Args>(args)...);
  }
  using func_pointer_t = Ret (*)(Method, const void *t, Args...);
  func_pointer_t func{nullptr};
};

/// VTable specialization for non const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...)>::template jump<T>)) {}

  constexpr VTableEntry() noexcept {}

  constexpr Ret operator()(Method m, void *t, Args... args) const noexcept {
    assert(t);
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  using func_pointer_t = Ret (*)(Method, void *t, Args...) noexcept;
  func_pointer_t func{nullptr};
};

/// VTable specialization for const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr VTableEntry() noexcept {}

  constexpr Ret operator()(Method m, const void *t,
                           Args... args) const noexcept {
    assert(t);
    return *func(m, t, std::forward<Args>(args)...);
  }

  using func_pointer_t = Ret (*)(Method, const void *t, Args...) noexcept;
  func_pointer_t func{nullptr};
};
/// @}

/// offset value type for interface vtbale
using method_offset_type = traits::smallest_uint_to_contain<POLY_MAX_METHOD_COUNT>;

/// complete vtable for a set of  @ref MethodSpec "method specs"
template <POLY_METHOD_SPEC... MethodSpecs>
struct VTable : public VTableEntry<MethodSpecs>... {
  using VTableEntry<MethodSpecs>::operator()...;
  template <typename T>
  constexpr VTable(poly::traits::Id<T> id) noexcept
      : VTableEntry<MethodSpecs>(id)... {}

  constexpr VTable() noexcept {};

  template <POLY_METHOD_SPEC Spec>
  static method_offset_type offset(traits::Id<Spec>) noexcept {
    constexpr VTable<MethodSpecs...> t;
    const std::byte *this_ =
        static_cast<const std::byte *>(static_cast<const void *>(&t));
    const std::byte *entry_ = static_cast<const std::byte *>(
        static_cast<const void *>(static_cast<const VTableEntry<Spec> *>(&t)));
    return static_cast<method_offset_type>(entry_ - this_);
  }
};

/// object vtable for T and a set of @ref MethodSpec "method specs"
template <typename T, POLY_METHOD_SPEC... MethodSpecs>
inline constexpr VTable<MethodSpecs...> vtable_for =
    VTable<MethodSpecs...>(poly::traits::Id<T>{});

/// The MethodContainer holds a pointer to the vtable, or is empty if the
/// provided list of MethodSpecs is empty.
/// @tparam Self the interface type that inherits from MethodContainer
/// @tparam ListOfSpecs TypeList of MethodSpecs
template <typename Self, POLY_TYPE_LIST ListOfSpecs, POLY_TYPE_LIST>
class MethodContainerImpl;
template <typename Self, template <typename...> typename List,
          POLY_METHOD_SPEC... MethodSpecs, typename... CollapsedOverloads>
class MethodContainerImpl<Self, List<MethodSpecs...>,
                          List<CollapsedOverloads...>>
    : public MethodInjector<CollapsedOverloads, Self>... {
public:
  template <typename MethodName, typename... Args>
  static constexpr bool nothrow_callable =
      noexcept((*std::declval<const VTable<MethodSpecs...> *>())(
          MethodName{}, std::declval<void *>(), std::declval<Args>()...));

  constexpr MethodContainerImpl() noexcept : vtable_(nullptr) {}
  constexpr MethodContainerImpl(const VTable<MethodSpecs...> *vtable) noexcept
      : vtable_(vtable) {}

  template <typename T>
  constexpr MethodContainerImpl(traits::Id<T>) noexcept
      : vtable_(&vtable_for<T, MethodSpecs...>) {}

  template <typename MethodName, typename... Args>
  constexpr decltype(auto)
  call(Args &&...args) noexcept(nothrow_callable<MethodName, Args...>) {
    assert(vtable_);
    return (*vtable_)(MethodName{}, self().data(), std::forward<Args>(args)...);
  }

  template <typename MethodName, typename... Args>
  constexpr decltype(auto) call(Args &&...args) const
      noexcept(nothrow_callable<MethodName, Args...>) {
    assert(vtable_);
    return (*vtable_)(MethodName{}, self().data(), std::forward<Args>(args)...);
  }

protected:
  template <typename T> constexpr void set_vtable(traits::Id<T>) {
    vtable_ = &vtable_for<T, MethodSpecs...>;
  }

  constexpr void set_vtable(const VTable<MethodSpecs...> *vtable) noexcept {
    vtable_ = vtable;
  }

  constexpr const VTable<MethodSpecs...> *vtable() const noexcept {
    return vtable_;
  }

  constexpr void reset_vtable() noexcept { vtable_ = nullptr; }

private:
  constexpr Self &self() noexcept { return *static_cast<Self *>(this); }
  constexpr const Self &self() const noexcept {
    return *static_cast<const Self *>(this);
  }
  const VTable<MethodSpecs...> *vtable_;
};
template <typename Self, template <typename...> typename List>
class MethodContainerImpl<Self, List<>, List<>> {
protected:
  constexpr void set_vtable(const void *) noexcept {}

  constexpr const void *vtable() const noexcept { return nullptr; }

  constexpr void reset_vtable() noexcept {}
};
template <typename Self, POLY_TYPE_LIST ListOfMethodSpecs>
using MethodContainer =
    MethodContainerImpl<Self, ListOfMethodSpecs,
                        typename collapse_overloads<ListOfMethodSpecs>::type>;

}
#endif
