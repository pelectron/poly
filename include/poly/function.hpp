#ifndef POLY_FUNCTION_HPP
#define POLY_FUNCTION_HPP
#include "poly/storage.hpp"
#include "poly/traits.hpp"
#include <cassert>

namespace poly {
template <typename Sig, typename Derived> class function_impl;
template <typename Sig, typename Storage> class function;

namespace detail {

template <typename Sig> struct invoke_ptr;
template <typename Ret, typename... Args> struct invoke_ptr<Ret(Args...)> {
  using type = Ret (*)(void *, Args...);
  template <typename F>
  static constexpr type value = +[](void *obj, Args... args) -> Ret {
    return (*static_cast<F *>(obj))(std::forward<Args>(args)...);
  };
  template <typename Storage> using once_type = Ret (*)(Storage &, Args...);
  template <typename Storage, typename F>
  static constexpr once_type<Storage> once =
      +[](Storage &storage, Args... args) -> Ret {
    F f = std::move(*static_cast<F *>(storage.data()));
    storage = Storage{};
    return f(std::forward<Args>(args)...);
  };
};
template <typename Ret, typename... Args>
struct invoke_ptr<Ret(Args...) const> {
  using type = Ret (*)(const void *, Args...);
  template <typename F>
  static constexpr type value = +[](const void *obj, Args... args) -> Ret {
    return (*static_cast<const F *>(obj))(std::forward<Args>(args)...);
  };

  template <typename Storage> using once_type = Ret (*)(Storage &, Args...);
  template <typename Storage, typename F>
  static constexpr once_type<Storage> once =
      +[](Storage &storage, Args... args) -> Ret {
    const F f = std::move(*static_cast<F *>(storage.data()));
    storage = Storage{};
    return f(std::forward<Args>(args)...);
  };
};
template <typename Ret, typename... Args>
struct invoke_ptr<Ret(Args...) noexcept> {
  using type = Ret (*)(void *, Args...) noexcept;
  template <typename F>
  static constexpr type value = +[](void *obj, Args... args) noexcept -> Ret {
    return (*static_cast<F *>(obj))(std::forward<Args>(args)...);
  };
  template <typename Storage>
  using once_type = Ret (*)(Storage &, Args...) noexcept;
  template <typename Storage, typename F>
  static constexpr once_type<Storage> once =
      +[](Storage &storage, Args... args) noexcept -> Ret {
    F f = std::move(*static_cast<F *>(storage.data()));
    storage = Storage{};
    return f(std::forward<Args>(args)...);
  };
};
template <typename Ret, typename... Args>
struct invoke_ptr<Ret(Args...) const noexcept> {
  using type = Ret (*)(const void *, Args...) noexcept;
  template <typename F>
  static constexpr auto value =
      +[](const void *obj, Args... args) noexcept -> Ret {
    return (*static_cast<const F *>(obj))(std::forward<Args>(args)...);
  };
  template <typename Storage>
  using once_type = Ret (*)(Storage &, Args...) noexcept;
  template <typename Storage, typename F>
  static constexpr once_type<Storage> once =
      +[](Storage &storage, Args... args) noexcept -> Ret {
    const F f = std::move(*static_cast<F *>(storage.data()));
    storage = Storage{};
    return f(std::forward<Args>(args)...);
  };
};
template <typename SigList, typename ArgList> struct resolve_signature;
template <typename... Args>
struct resolve_signature<type_list<>, type_list<Args...>> {
  using type = void;
};
template <typename Sig, typename... Sigs, typename... Args>
struct resolve_signature<type_list<Sig, Sigs...>, type_list<Args...>> {
  using type = std::conditional_t<
      std::is_same_v<
          transform_t<typename traits::func_args<Sig>::type, std::decay>,
          type_list<std::decay_t<Args>...>>,
      Sig,
      typename resolve_signature<type_list<Sigs...>, type_list<Args...>>::type>;
};
template <typename T> struct is_poly_function : std::false_type {};
template <typename Sig, typename Derived>
struct is_poly_function<function_impl<Sig, Derived>> : std::true_type {};
template <typename Sig, typename Storage>
struct is_poly_function<function<Sig, Storage>> : std::true_type {};
template <typename Sig, typename Storage> class basic_function;
template <typename Sig, typename Storage>
struct is_poly_function<basic_function<Sig, Storage>> : std::true_type {};

template <typename Sig, typename Storage> class basic_function {
public:
  static_assert(poly::is_storage_v<Storage>,
                "Storage must conform to the poly::Storage concept");
  using return_type = typename traits::func_return_type<Sig>::type;
  using argument_types = typename traits::func_args<Sig>::type;
  using invoke_ptr_t = typename detail::invoke_ptr<Sig>::type;

  static constexpr bool is_const = traits::func_is_const<Sig>::value;
  static constexpr bool is_nothrow = traits::func_is_noexcept<Sig>::value;
  template <typename F>
  static constexpr bool nothrow_emplacable =
      noexcept(std::declval<Storage>().template emplace<std::decay_t<F>>(
          std::forward<F>(std::declval<F>())));

  template <typename F,
            typename = std::enable_if_t<
                not detail::is_poly_function<std::decay_t<F>>::value>>
  constexpr basic_function(F &&f) noexcept(nothrow_emplacable<F>) {
    bind(std::forward<F>(f));
  }

  constexpr basic_function() noexcept = default;
  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<Storage, const OtherStorage &>>>
  constexpr basic_function(
      const basic_function<Sig, OtherStorage> &
          other) noexcept(std::is_nothrow_constructible_v<Storage,
                                                          const OtherStorage &>)
      : invoke_(other.invoke_), storage_(other.storage_) {}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_constructible_v<Storage, OtherStorage &&>>>
  constexpr basic_function(basic_function<Sig, OtherStorage> &&other) noexcept(
      std::is_nothrow_constructible_v<Storage, OtherStorage &&>)
      : invoke_(other.invoke_), storage_(std::move(other.storage_)) {}

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<Storage, const OtherStorage &>>>
  constexpr basic_function &
  operator=(const basic_function<Sig, OtherStorage> &other) noexcept(
      std::is_nothrow_assignable_v<Storage, const OtherStorage &>) {
    storage_ = other.storage_;
    invoke_ = other.invoke_;
    return *this;
  }

  template <typename OtherStorage,
            typename = std::enable_if_t<
                std::is_assignable_v<Storage, OtherStorage &&>>>
  constexpr basic_function &
  operator=(basic_function<Sig, OtherStorage> &&other) noexcept(
      std::is_nothrow_assignable_v<Storage, OtherStorage &&>) {
    storage_ = std::move(other.storage_);
    invoke_ = other.invoke_;
    return *this;
  }
  template <typename... Args>
  constexpr return_type operator()(Args &&...args) noexcept(is_nothrow) {
    assert(storage_.data());
    assert(invoke_);
    static_assert(
        std::is_invocable_v<invoke_ptr_t, void *, decltype(args)...>,
        "This function cannot be callled with the provided arguments.");
    return (*invoke_)(storage_.data(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  constexpr return_type operator()(Args &&...args) const noexcept(is_nothrow) {
    assert(storage_.data());
    assert(invoke_);
    return (*invoke_)(storage_.data(), std::forward<Args>(args)...);
  }

  template <typename F> void bind(F &&f) noexcept(nothrow_emplacable<F>) {
    static_assert(poly::is_invocable_v<Sig, std::decay_t<F>>,
                  "f is not callable with the signature defined");
    storage_.template emplace<std::decay_t<F>>(std::forward<F>(f));
    invoke_ = detail::invoke_ptr<Sig>::template value<std::decay_t<F>>;
  }

private:
  invoke_ptr_t invoke_{nullptr};
  Storage storage_;
};
} // namespace detail

template <typename Sig, typename Derived> class function_impl {
public:
  using return_type = typename traits::func_return_type<Sig>::type;
  using argument_type = typename traits::func_args<Sig>::type;
  using invoke_ptr_t = typename detail::invoke_ptr<Sig>::type;

  static constexpr bool is_const = traits::func_is_const<Sig>::value;
  static constexpr bool is_nothrow = traits::func_is_noexcept<Sig>::value;

  template <typename Storage, typename... Sigs> friend class any_function;
  template <typename Sig_, typename Storage> friend class once_function;
  template <typename F,
            typename = std::enable_if_t<
                not detail::is_poly_function<std::decay_t<F>>::value>>
  constexpr function_impl(traits::Id<F>) noexcept
      : invoke_(detail::invoke_ptr<Sig>::template value<std::decay_t<F>>) {}

  constexpr function_impl(invoke_ptr_t invoke) noexcept : invoke_(invoke) {}
  constexpr function_impl() noexcept = default;
  constexpr function_impl(const function_impl &) noexcept = default;
  constexpr function_impl(function_impl &&) noexcept = default;
  constexpr function_impl &operator=(const function_impl &) noexcept = default;
  constexpr function_impl &operator=(function_impl &&) noexcept = default;

  template <typename... Args>
  constexpr return_type operator()(Args &&...args) noexcept(is_nothrow) {
    void *data = derived().data();
    return (*invoke_)(data, std::forward<Args>(args)...);
  }

  template <typename... Args>
  constexpr return_type operator()(Args &&...args) const noexcept(is_nothrow) {
    const void *data = derived().data();
    return (*invoke_)(data, std::forward<Args>(args)...);
  }

  template <typename F> void bind(F &&f) {
    static_assert(is_invocable_v<Sig, std::decay_t<F>>,
                  "f is not callable with the signature defined");
    derived().template emplace<std::decay_t<F>>(std::forward<F>(f));
    invoke_ = detail::invoke_ptr<Sig>::template value<std::decay_t<F>>;
  }

private:
  Derived &derived() noexcept { return static_cast<Derived &>(*this); }
  const Derived &derived() const noexcept {
    return static_cast<const Derived &>(*this);
  }

  invoke_ptr_t invoke_{nullptr};
};
template <typename Sig, typename Storage> class function;
template <typename Storage, typename Ret, typename... Args>
class function<Ret(Args...), Storage>
    : public detail::basic_function<Ret(Args...), Storage> {
public:
  using Base = detail::basic_function<Ret(Args...), Storage>;
  using Base::Base;
  using Base::operator=;

  Ret operator()(Args... args) {
    return Base::operator()(std::forward<Args>(args)...);
  }
};

template <typename Storage, typename Ret, typename... Args>
class function<Ret(Args...) noexcept, Storage>
    : public detail::basic_function<Ret(Args...) noexcept, Storage> {
public:
  using Base = detail::basic_function<Ret(Args...) noexcept, Storage>;
  using Base::Base;
  using Base::operator=;

  Ret operator()(Args... args) noexcept {
    return Base::operator()(std::forward<Args>(args)...);
  }
};

template <typename Storage, typename Ret, typename... Args>
class function<Ret(Args...) const, Storage>
    : public detail::basic_function<Ret(Args...) const, Storage> {
public:
  using Base = detail::basic_function<Ret(Args...) const, Storage>;
  using Base::Base;
  using Base::operator=;

  Ret operator()(Args... args) const {
    return Base::operator()(std::forward<Args>(args)...);
  }
};

template <typename Storage, typename Ret, typename... Args>
class function<Ret(Args...) const noexcept, Storage>
    : public detail::basic_function<Ret(Args...) const noexcept, Storage> {
public:
  using Base = detail::basic_function<Ret(Args...) const noexcept, Storage>;
  using Base::Base;
  using Base::operator=;

  Ret operator()(Args... args) const noexcept {
    return Base::operator()(std::forward<Args>(args)...);
  }
};
template <typename Storage, typename... Sigs>
class any_function
    : public function_impl<Sigs, any_function<Storage, Sigs...>>... {
public:
  template <typename... Args>
  static constexpr bool is_nothrow_invocable =
      traits::func_is_noexcept<typename detail::resolve_signature<
          type_list<Sigs...>, type_list<Args...>>::type>::value;

  template <typename... Args>
  static constexpr bool is_const_invocable =
      traits::func_is_const<typename detail::resolve_signature<
          type_list<Sigs...>, type_list<Args...>>::type>::value;

  template <typename... Args>
  using return_type_for =
      typename traits::func_return_type<typename detail::resolve_signature<
          type_list<Sigs...>, type_list<Args...>>::type>::type;

  template <typename F,
            typename = std::enable_if_t<
                not detail::is_poly_function<std::decay_t<F>>::value>>
  constexpr any_function(F &&f) {
    static_assert((is_invocable_v<Sigs, std::decay_t<F>> or ...),
                  "f is not callable with any of the signatures defined");
    this->bind(std::forward<F>(f));
  }
  constexpr any_function(const any_function &other) = default;
  constexpr any_function(any_function &&other) = default;
  constexpr any_function &operator=(const any_function &other) = default;
  constexpr any_function &operator=(any_function &&other) = default;
  template <typename F> void bind(F &&f) {
    ((static_cast<function_impl<Sigs, any_function<Storage, Sigs...>> *>(this)
          ->invoke_ =
          detail::invoke_ptr<Sigs>::template value<std::decay_t<F>>),
     ...);
    storage_.template emplace<std::decay_t<F>>(std::forward<F>(f));
  }

  template <typename... Args,
            typename = std::enable_if_t<not is_const_invocable<Args...>>>
  constexpr return_type_for<Args...>
  operator()(Args &&...args) noexcept(is_nothrow_invocable<Args...>) {
    using Sig =
        typename detail::resolve_signature<type_list<Sigs...>,
                                           type_list<decltype(args)...>>::type;
    return (*static_cast<function_impl<Sig, any_function<Storage, Sigs...>> *>(
        this))(std::forward<Args>(args)...);
  }

  template <typename... Args,
            typename = std::enable_if_t<is_const_invocable<Args...>>>
  constexpr return_type_for<Args...> operator()(Args &&...args) const
      noexcept(is_nothrow_invocable<Args...>) {
    using Sig = typename detail::resolve_signature<type_list<Sigs...>,
                                                   type_list<Args...>>::type;
    return (*static_cast<function_impl<Sig, any_function<Storage, Sigs...>> *>(
        this))(std::forward<Args>(args)...);
  }

private:
  template <typename S, typename D> friend class function_impl;

  void *data() noexcept { return storage_.data(); }

  const void *data() const noexcept { return storage_.data(); }

  Storage storage_{};
};

template <typename Sig, typename Storage> class once_function {
public:
  using return_type = typename traits::func_return_type<Sig>::type;
  using argument_types = typename traits::func_args<Sig>::type;
  using invoke_ptr_t =
      typename detail::invoke_ptr<Sig>::template once_type<Storage>;

  static constexpr bool is_nothrow = traits::func_is_noexcept<Sig>::value;

  template <typename F,
            typename = std::enable_if_t<
                not detail::is_poly_function<std::decay_t<F>>::value>>
  constexpr once_function(F &&f) {
    bind(std::forward<F>(f));
  }

  constexpr once_function(const once_function &other) = default;
  constexpr once_function(once_function &&other) = default;
  constexpr once_function &operator=(const once_function &other) = default;
  constexpr once_function &operator=(once_function &&other) = default;
  template <typename F,
            typename = std::enable_if_t<
                not detail::is_poly_function<std::decay_t<F>>::value>>
  void bind(F &&f) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<F>, decltype(f)>) {
    storage_.template emplace<std::decay_t<F>>(std::forward<F>(f));
    this->invoke_ = detail::invoke_ptr<Sig>::template once<std::decay_t<F>>;
  }

  template <typename... Args>
  constexpr return_type operator()(Args &&...args) noexcept(is_nothrow) {
    return (*invoke_)(storage_, std::forward<Args>(args)...);
  }

private:
  template <typename S, typename D> friend class function_impl;

  void *data() noexcept { return storage_.data(); }

  const void *data() const noexcept { return storage_.data(); }

  template <typename T, typename... Args> T &emplace(Args &&...args) {
    return storage_.template emplace<T>(std::forward<Args>(args)...);
  }
  invoke_ptr_t invoke_;
  Storage storage_{};
};
} // namespace poly
#endif
