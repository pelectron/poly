/**
 *  Copyright 2024 Pelé Constam
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef POLY_METHOD_HPP
#define POLY_METHOD_HPP
#include "poly/always_false.hpp"
#include "poly/config.hpp"
#include "poly/traits.hpp"
#include <type_traits>

namespace poly {

/// @addtogroup method_extension Method Extension
/// @ref MethodSpec "MethodSpecs" for an arbitrary type T are implemented by
/// defining a function called extend.
///
/// extend must be locatable through "argument dependent lookup" (ADL), that
/// is, extend should be defined in the same namespace as the method name, or
/// the same namespace as the T to be extended. In general, if the T is a type
/// owned by the user, extend should be in the same namespace as T. For third
/// party types, such as standard library containers, extend should be defined
/// in the same namespace as the methods name.
///
/// The signature of extend must match the @ref MethodSpec. For a @ref
/// MethodSpec with return type Ret, name Name, and arguments Args..., extend
/// must have the following signature, where T is the type extended by
/// extend() and obj is the instance of T the method is called upon:
///
/// Ret extend(Name, T& obj, Args... args);
///
/// If the @ref MethodSpec is const, extend must take a const reference to the
/// object:
///
/// Ret extend(Name, const T& obj, Args...args);
///
/// If the @ref MethodSpec is specified noexcept, extend must be noexcept
/// also.
///
/// To enable name injection, the @ref POLY_METHOD macro has to be used to
/// define the method name.
/// @{

/// extension point for the @ref MethodSpec
/// 'Ret(MethodName,Args...)[noexcept]'.
template<typename Ret, typename MethodName, typename T, typename... Args,
         typename = std::enable_if_t<detail::always_false<T>>>
Ret extend(MethodName, T& t, Args... args);

/// extension point for the @ref MethodSpec 'Ret(MethodName,Args...)const
/// [noexcept]'.
template<typename Ret, typename MethodName, typename T, typename... Args,
         typename = std::enable_if_t<detail::always_false<T>>>
Ret extend(MethodName, const T& t, Args... args);
/// @}

/// @ingroup  method_extension
/// @def POLY_METHOD(MethodName)
/// Defines the method name MethodName.
///
/// If method injection is disabled, this macro will simply expand to
///
/// ```
/// _this_type{};
/// ```
///
/// If injection is enabled, the struct defined will also contain an inner
/// class template called injector, i.e. the macro wil expand to:
///
/// ```
/// _this_type{
/// template<typenameSelf,typename Spec>
/// struct injector{};
/// };
/// ```
///
/// If default extension is enabled, a generic extend() function for
/// MethodName is defined in the following way:
///
/// ```
/// template<typename T,typename...Args>
/// decltype(auto) extend(MethodName, T& t, Args&&...args){
///   return t.MethodName(std::forward<Args>(args)...);
/// }
/// ```
} // namespace poly

#if POLY_USE_MACROS

#  define POLY_METHODS(...) poly::type_list<__VA_ARGS__>

#  define POLY_METHOD(MethodName) \
    POLY_METHOD_IMPL(MethodName)  \
    POLY_DEFAULT_EXTEND_IMPL(MethodName)

#  if POLY_USE_METHOD_INJECTOR

#    define POLY_METHOD_IMPL(MethodName)                                       \
      struct MethodName {                                                      \
        using _this_type = MethodName;                                         \
        template<typename Self, typename MethodSpecOrListOfSpecs>              \
        struct POLY_EMPTY_BASE injector;                                       \
                                                                               \
        /** specialization for non overloaded method*/                         \
        template<typename Self, typename Ret, typename... Args>                \
        struct POLY_EMPTY_BASE injector<Self, Ret(MethodName, Args...)> {      \
          constexpr Ret MethodName(Args... args) {                             \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, typename Ret, typename... Args>                \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, Ret(MethodName, Args...) const> {                   \
          constexpr Ret MethodName(Args... args) const {                       \
            const Self* self = static_cast<const Self*>(this);                 \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, typename Ret, typename... Args>                \
        struct injector<Self, Ret(MethodName, Args...) noexcept> {             \
          constexpr Ret MethodName(Args... args) noexcept {                    \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, typename Ret, typename... Args>                \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, Ret(MethodName, Args...) const noexcept> {          \
          constexpr Ret MethodName(Args... args) const noexcept {              \
            const Self* self = static_cast<const Self*>(this);                 \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        /** specialization for overloaded methods */                           \
        /** cannot use variadic inheritance because of empty base optimization \
         */                                                                    \
        /** issues -> linear single inheritance */                             \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args, typename... Specs>            \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...), Specs...>>           \
            : public injector<Self, List<Specs...>> {                          \
          using injector<Self, List<Specs...>>::MethodName;                    \
                                                                               \
          Ret MethodName(Args... args) {                                       \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args, typename... Specs>            \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...) noexcept, Specs...>>  \
            : public injector<Self, List<Specs...>> {                          \
          using injector<Self, List<Specs...>>::MethodName;                    \
                                                                               \
          Ret MethodName(Args... args) noexcept {                              \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args, typename... Specs>            \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...) const, Specs...>>     \
            : public injector<Self, List<Specs...>> {                          \
          using injector<Self, List<Specs...>>::MethodName;                    \
                                                                               \
          Ret MethodName(Args... args) const {                                 \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args, typename... Specs>            \
        struct POLY_EMPTY_BASE injector<                                       \
            Self, List<Ret(MethodName, Args...) const noexcept, Specs...>>     \
            : public injector<Self, List<Specs...>> {                          \
          using injector<Self, List<Specs...>>::MethodName;                    \
                                                                               \
          Ret MethodName(Args... args) const noexcept {                        \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args>                               \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...)>> {                   \
          Ret MethodName(Args... args) {                                       \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args>                               \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...) noexcept>> {          \
                                                                               \
          Ret MethodName(Args... args) noexcept {                              \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args>                               \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...) const>> {             \
          Ret MethodName(Args... args) const {                                 \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
                                                                               \
        template<typename Self, template<typename...> typename List,           \
                 typename Ret, typename... Args>                               \
        struct POLY_EMPTY_BASE                                                 \
            injector<Self, List<Ret(MethodName, Args...) const noexcept>> {    \
          Ret MethodName(Args... args) const noexcept {                        \
            Self* self = static_cast<Self*>(this);                             \
            return self->template call<_this_type>(                            \
                std::forward<Args>(args)...);                                  \
          }                                                                    \
        };                                                                     \
      };

#  else
#    define POLY_METHOD_IMPL(MethodName) \
      struct MethodName {};
#  endif

#  if POLY_USE_DEFAULT_EXTEND
#    define POLY_DEFAULT_EXTEND_IMPL(MethodName)                              \
      template<typename T, typename... Args>                                  \
      decltype(auto) extend(MethodName, T& t, Args&&... args) noexcept(       \
          noexcept(std::declval<T>().MethodName(                              \
              std::forward<Args>(std::declval<decltype(args)>())...))) {      \
        return t.MethodName(std::forward<Args>(args)...);                     \
      }                                                                       \
                                                                              \
      template<typename T, typename... Args>                                  \
      decltype(auto) extend(MethodName, const T& t, Args&&... args) noexcept( \
          noexcept(std::declval<const T>().MethodName(                        \
              std::forward<Args>(std::declval<decltype(args)>())...))) {      \
        return t.MethodName(std::forward<Args>(args)...);                     \
      }
#  else
#    define POLY_DEFAULT_EXTEND_IMPL(MethodName)
#  endif

#endif

#endif
