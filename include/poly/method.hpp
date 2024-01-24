#ifndef POLY_METHOD_HPP
#define POLY_METHOD_HPP
#include "poly/config.hpp"
#include "poly/traits.hpp"
#include "poly/type_list.hpp"

#include <cassert>
#include <type_traits>

#if POLY_USE_MACROS == 1

#define POLY_METHOD(MethodName)                                                \
  POLY_METHOD_IMPL(MethodName)                                                 \
  POLY_DEFAULT_EXTEND_IMPL(MethodName)

#if (POLY_USE_INJECTOR == 1) || (POLY_USE_METHOD_INJECTOR == 1)

#define POLY_METHOD_IMPL(MethodName)                                           \
  struct MethodName {                                                          \
    template <typename Self, typename MethodSpecOrListOfSpecs>                 \
    struct injector;                                                           \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...)> {                          \
      Ret MethodName(Args... args) {                                           \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) const> {                    \
      Ret MethodName(Args... args) const {                                     \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) noexcept> {                 \
      Ret MethodName(Args... args) noexcept {                                  \
        Self *self = static_cast<Self *>(this);                                \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename Ret, typename... Args>                   \
    struct injector<Self, Ret(MethodName, Args...) const noexcept> {           \
      Ret MethodName(Args... args) const noexcept {                            \
        const Self *self = static_cast<const Self *>(this);                    \
        return self->template call<struct MethodName>(                         \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    };                                                                         \
    template <typename Self, typename... MethodSpecs>                          \
    struct injector<Self, poly::type_list<MethodSpecs...>>                     \
        : public injector<Self, MethodSpecs>... {                              \
      using injector<Self, MethodSpecs>::MethodName...;                        \
    };                                                                         \
  };

#else
#define POLY_METHOD_IMPL(MethodName)                                           \
  struct MethodName {};
#endif

#if POLY_USE_DEFAULT_EXTEND == 1
#define POLY_DEFAULT_EXTEND_IMPL(MethodName)                                   \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(MethodName, T &t, Args &&...args) noexcept(            \
      noexcept(std::declval<T>().MethodName(                                   \
          std::forward<Args>(std::declval<decltype(args)>())...))) {           \
    return t.MethodName(std::forward<Args>(args)...);                          \
  }                                                                            \
                                                                               \
  template <typename T, typename... Args>                                      \
  decltype(auto) extend(MethodName, const T &t, Args &&...args) noexcept(      \
      noexcept(std::declval<const T>().MethodName(                             \
          std::forward<Args>(std::declval<decltype(args)>())...))) {           \
    return t.MethodName(std::forward<Args>(args)...);                          \
  }
#else
#define POLY_DEFAULT_EXTEND_IMPL(MethodName)
#endif

#endif

namespace poly {

/// @addtogroup MethodSpec
/// @{
/// extension point for the MethodSpec 'Ret(MethodName,Args...)[noexcept]'.
template <typename Ret, typename MethodName, typename T, typename... Args>
Ret extend(MethodName, T &t, Args &&...args);

/// extension point for the MethodSpec 'Ret(MethodName,Args...)const
/// [noexcept]'.
template <typename Ret, typename MethodName, typename T, typename... Args>
Ret extend(MethodName, const T &t, Args &&...args);
/// @}

namespace detail {

/// The MethodInjector is used to inject methods by name.
/// @{
/// default method injector does nothing
template <typename MethodSpecOrListOfSpecs, typename Self, typename = void>
struct MethodInjector {};

/// if the method was created with the POLY_METHOD macro, there will be an
/// innner template named "injector" present, which is templated on a type Self
/// and the MethodSpec. The injector will link the Method with its real name,
/// i.e. "Method". This allows the end user to use the syntax
/// obj.Method(args...) instead of obj.call<Method>(args...).
/// @{
template <typename Spec, typename Self>
struct MethodInjector<
    Spec, Self,
    std::void_t<typename method_name_t<Spec>::template injector<Self, Spec>>>
    : public method_name_t<Spec>::template injector<Self, Spec> {};

/// sepcialization for overloaded methods
template <typename... MethodSpecs, typename Self>
struct MethodInjector<
    poly::type_list<MethodSpecs...>, Self,
    std::void_t<
        typename poly::method_name_t<at_t<poly::type_list<MethodSpecs...>, 0>>::
            template injector<Self, poly::type_list<MethodSpecs...>>>>
    : public poly::method_name_t<at_t<poly::type_list<MethodSpecs...>, 0>>::
          template injector<Self, poly::type_list<MethodSpecs...>> {};
/// @}
/// @}

template <typename Name, typename SpecList> struct build_method_set;
template <typename Name, typename... Specs>
struct build_method_set<Name, type_list<Specs...>> {
  template <typename T> using predicate = std::is_same<Name, method_name_t<T>>;
  template <typename T> struct neg_predicate {
    static constexpr bool value = not predicate<T>::value;
  };

  using overloaded_specs = filter_t<type_list<Specs...>, predicate>;
  // spec to add to output. if not an overload, add the overloaded_specs
  // directly to output(unwrapped in build_method_sets), else wrap the overload
  // list in another type_list.
  using spec =
      std::conditional_t<list_size<overloaded_specs>::value == 1,
                         overloaded_specs, type_list<overloaded_specs>>;
  // the remaining specs which did not match Name
  using output_list = filter_t<type_list<Specs...>, neg_predicate>;
};

template <typename NameList, typename SpecList> struct build_method_sets;
template <> struct build_method_sets<type_list<>, type_list<>> {
  using type = type_list<>;
};
template <typename Name, typename... Names, typename... Specs>
struct build_method_sets<type_list<Name, Names...>, type_list<Specs...>> {
  using bms = build_method_set<Name, type_list<Specs...>>;
  using rest =
      build_method_sets<type_list<Names...>, typename bms::output_list>;
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
template <typename MethodSpecList> struct collapse_overloads;
template <> struct collapse_overloads<type_list<>> {
  using type = type_list<>;
};
template <typename... MethodSpecs>
struct collapse_overloads<type_list<MethodSpecs...>>
    : build_method_sets<typename remove_duplicates<
                            type_list<method_name_t<MethodSpecs>...>>::type,
                        type_list<MethodSpecs...>> {};

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
template <typename MethodSpec> struct trampoline;

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

  constexpr Ret operator()(Method m, void *t, Args... args) const {
    assert(t);
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, void *t, Args...);
};

/// VTable specialization for const method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr Ret operator()(Method m, const void *t, Args... args) const {
    assert(t);
    return *func(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, const void *t, Args...);
};

/// VTable specialization for non const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...)>::template jump<T>)) {}

  constexpr Ret operator()(Method m, void *t, Args... args) const noexcept {
    assert(t);
    return (*func)(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, void *t, Args...) noexcept;
};

/// VTable specialization for const noexcept method
template <typename Ret, typename Method, typename... Args>
struct VTableEntry<Ret(Method, Args...) const noexcept> {
  template <typename T>
  constexpr VTableEntry(poly::traits::Id<T>) noexcept
      : func(std::addressof(
            trampoline<Ret(Method, Args...) const>::template jump<T>)) {}

  constexpr Ret operator()(Method m, const void *t,
                           Args... args) const noexcept {
    assert(t);
    return *func(m, t, std::forward<Args>(args)...);
  }
  Ret (*func)(Method, const void *t, Args...) noexcept;
};
/// @}

/// complete vtable for a set of  @ref MethodSpec "method specs"
template <typename... MethodSpecs>
struct VTable : private VTableEntry<MethodSpecs>... {
  using VTableEntry<MethodSpecs>::operator()...;
  template <typename T>
  constexpr VTable(poly::traits::Id<T> id) noexcept
      : VTableEntry<MethodSpecs>(id)... {}
};

/// vtable for T and a set of @ref MethodSpec "method specs"
template <typename T, typename... MethodSpecs>
inline constexpr VTable<MethodSpecs...> vtable_for =
    VTable<MethodSpecs...>(poly::traits::Id<T>{});

/// The MethodContainer holds a pointer to the vtable, or is empty if the
/// provided list of MethodSpecs is empty.
/// @tparam Self the interface type that inherits from MethodContainer
/// @tparam ListOfSpecs type_list of MethodSpecs
template <typename Self, typename ListOfSpecs, typename>
class MethodContainerImpl;
template <typename Self, typename... MethodSpecs,
          typename... CollapsedOverloads>
class MethodContainerImpl<Self, type_list<MethodSpecs...>,
                          type_list<CollapsedOverloads...>>
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
template <typename Self>
class MethodContainerImpl<Self, type_list<>, type_list<>> {
protected:
  constexpr void set_vtable(const void *) noexcept {}

  constexpr const void *vtable() const noexcept { return nullptr; }

  constexpr void reset_vtable() noexcept {}
};
template <typename Self, typename ListOfMethodSpecs>
using MethodContainer =
    MethodContainerImpl<Self, ListOfMethodSpecs,
                        typename collapse_overloads<ListOfMethodSpecs>::type>;

} // namespace detail
} // namespace poly
#endif
