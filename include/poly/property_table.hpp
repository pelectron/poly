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
#ifndef POLY_OBJECT_PROPERTY_HPP
#define POLY_OBJECT_PROPERTY_HPP
#include "poly/config.hpp"
#include "poly/property.hpp"
#include "poly/traits.hpp"

#include <utility>

namespace poly {
namespace detail {
  using property_offset_type =
      traits::smallest_uint_to_contain<poly::config::max_property_count - 1>;

  template<typename Self, POLY_PROP_SPEC PropertySpec>
  struct NullPropertyInjector {};
  /// type trait to detect inner template "injector" in the name of the
  /// PropertySpec
  /// @{
  template<typename Self, POLY_PROP_SPEC PropertySpec, typename = void>
  struct has_property_injector : std::false_type {};
  template<typename Self, POLY_PROP_SPEC PropertySpec>
  struct has_property_injector<
      Self, PropertySpec,
      std::void_t<typename property_name_t<PropertySpec>::template injector<
          Self, PropertySpec>>> : std::true_type {};
  /// @}

  /// The PropertyInjector class eitheris an empty base class, or inherits
  /// from the property names inner template "injector"
  /// @{
  /// default injector does nothing
  template<typename Self, POLY_PROP_SPEC PropertySpec, bool hasInjector = false>
  struct get_property_injector {
    using type = NullPropertyInjector<Self, PropertySpec>;
  };
  /// if the Property was created with the POLY_PROPERTY macro, the name is
  /// injected and the property can be set with obj.PropertyName = value and
  /// retrieved with obj.PropertyName
  template<typename Self, POLY_PROP_SPEC PropertySpec>
  struct get_property_injector<Self, PropertySpec, true> {
    using type =
        property_name_t<PropertySpec>::template injector<Self, PropertySpec>;
  };

  template< typename Self,POLY_PROP_SPEC PropertySpec>
  using property_injector_for_t = typename get_property_injector<
      Self, PropertySpec,
      has_property_injector<Self, PropertySpec>::value>::type;
  /// @}

  /// The type of the injected member. It provides a conversion operator to
  /// Type (i.e. get()), and an assignment operator for a T convertible to
  /// Type if the property is not const (i.e. set()).
  ///
  /// @tparam Self the type of the Object,
  /// @tparam Injector the type containing the InjectedProperty as a member.
  /// @tparam Name the name of the property
  /// @tparam Type the value type of the property
  /// @tparam Const specifies wether this property is const or not
  /// @tparam NoThrow specifies wether this property is non throwing or not
  /// @{
  template<typename Self, typename Injector, typename Name, typename Type,
           bool Const, bool NoThrow>
  class POLY_EMPTY_BASE InjectedProperty {
  public:
    using type = Type;

    operator Type() noexcept(NoThrow) {
      static_assert(not NoThrow or noexcept(self().template get<Name>()));
      return self().template get<Name>();
    }

    template<typename T, typename = std::enable_if_t<not std::is_same_v<
                             InjectedProperty, std::decay_t<T>>>>
    InjectedProperty& operator=(T&& t) noexcept(NoThrow) {
      self().template set<Name>(std::forward<T>(t));
      return *this;
    }

  private:
    Self& self() noexcept {
      // reinterpret_cast is valid and not UB. InjectedProperty is the only
      // member of Names injector. If injector is standard_layout, the this
      // pointer (Injector*) is interconvertible with the pointer to the first
      // member (this). That the PropertyInjector is standard_layout, and as
      // such all members of it, is statically asserted in the
      // PropertyContainer.
      return *static_cast<Self*>(reinterpret_cast<Injector*>(this));
    }
    const Self& self() const noexcept {
      // reinterpret_cast is valid and not UB. InjectedProperty is the only
      // member of Names injector. If injector is standard_layout, the this
      // pointer (Injector*) is interconvertible with the pointer to the first
      // member (this). That the PropertyInjector is standard_layout, and as
      // such all members of it, is statically asserted in the
      // PropertyContainer.
      return *static_cast<const Self*>(reinterpret_cast<const Injector*>(this));
    }
  };

  template<typename Self, typename Injector, typename Name, typename Type,
           bool NoThrow>
  class POLY_EMPTY_BASE
      InjectedProperty<Self, Injector, Name, Type, true, NoThrow> {
  public:
    using type = Type;

    template<typename S = Self, typename I = Injector>
    operator Type() noexcept(NoThrow) {
      static_assert(not NoThrow or noexcept(self().template get<Name>()));
      return self().template get<Name>();
    }

  private:
    const Self& self() const noexcept {
      // reinterpret_cast is valid and not UB. InjectedProperty is the only
      // member of Names injector. If injector is standard_layout, the this
      // pointer (Injector*) is interconvertible with the pointer to the first
      // member (this). That the PropertyInjector is standard_layout, and as
      // such all members of it, is statically asserted in the
      // PropertyContainer.
      return *static_cast<const Self*>(reinterpret_cast<const Injector*>(this));
    }
  };
  /// @}

  /// @addtogroup ptable Property Table
  /// @{
  /// Properties are accessed through a property table. It is a table of
  /// property_entry entries. The entries hold the type erased getter, and
  /// optional setter, for a property. A pointer to the property table is held
  /// by the PropertyContainer.

  /// Individual entry in the property table. Contains getter and optional
  /// setter.
  /// @{
  template<POLY_PROP_SPEC PropertySpec>
  struct property_entry;
  template<typename Name, typename Type>
  struct property_entry<const Name(Type)> {
    template<typename T>
    constexpr property_entry(poly::traits::Id<T>) noexcept
        : get_(+[](Name, const void* t) -> Type {
            using poly::get;
            return get(Name{}, *static_cast<const T*>(t));
          }) {}
    constexpr property_entry() noexcept = default;
    template<typename T>
    constexpr void set(Name, void*, const T&) const {
      static_assert(detail::always_false<T>,
                    "This property is not settable, i.e. defined as const.");
    }

    constexpr Type get(Name, const void* t) const {
      assert(get_);
      assert(t);
      return (*get_)(Name{}, t);
    }

    Type (*get_)(Name, const void*) = nullptr;
  };

  template<typename Name, typename Type>
  struct property_entry<const Name(Type) noexcept> {
    constexpr property_entry() noexcept = default;
    template<typename T>
    constexpr property_entry(poly::traits::Id<T>) noexcept
        : get_(+[](Name, const void* t) noexcept -> Type {
            using poly::get;
            static_assert(
                noexcept(get(std::declval<Name>(), std::declval<const T>())),
                "Property specified noexcept, but get(Name, const T&)->Type "
                "is "
                "not noexcept");
            return get(Name{}, *static_cast<const T*>(t));
          }) {}

    template<typename T>
    constexpr void set(Name, void*, const T&) const noexcept {
      static_assert(
          detail::always_false<T>,
          "This property is not settable, i.e. it is defined as const.");
    }

    constexpr Type get(Name, const void* t) const noexcept {
      assert(get_);
      assert(t);
      return (*get_)(Name{}, t);
    }

    Type (*get_)(Name, const void*) = nullptr;
  };
  template<typename Name, typename Type>
  struct property_entry<Name(Type)> {
    template<typename T>
    constexpr property_entry(poly::traits::Id<T>) noexcept
        : set_{+[](Name, void* t, const Type& value) -> bool {
            using poly::set;
            if constexpr (has_validator_v<T, Name(Type)>) {
              using poly::check;
              if (!check(Name{}, *static_cast<const T*>(t), value)) {
                return false;
              }
            }
            set(Name{}, *static_cast<T*>(t), value);
            return true;
          }},
          get_(+[](Name, const void* t) -> Type {
            using poly::get;
            return get(Name{}, *static_cast<const T*>(t));
          }) {}
    constexpr property_entry() noexcept = default;
    constexpr bool set(Name, void* t, const Type& value) const {
      assert(set_);
      assert(t);
      return (*set_)(Name{}, t, value);
    }
    constexpr Type get(Name, const void* t) const {
      assert(get_);
      assert(t);
      return (*get_)(Name{}, t);
    }

    bool (*set_)(Name, void*, const Type&) = nullptr;
    Type (*get_)(Name, const void*) = nullptr;
  };
  template<typename Name, typename Type>
  struct property_entry<Name(Type) noexcept> {
    template<typename T>
    constexpr property_entry(poly::traits::Id<T>) noexcept
        : set_{+[](Name, void* t, const Type& value) -> bool {
            using poly::set;
            static_assert(
                noexcept(set(std::declval<Name>(), std::declval<T&>())),
                "Property specified noexcept, but set(Name, T&) is "
                "not noexcept");
            if constexpr (has_validator_v<T, Name(Type)>) {
              using poly::check;
              static_assert(
                  noexcept(check(Name{},
                                 std::declval<const T&>(),
                                 std::declval<const Type&>())),
                  "Property specified noexcept, but check(Name, const "
                  "T&,const Type&) is not specified noexcept.");
              if (!check(Name{}, *static_cast<const T*>(t), value)) {
                return false;
              }
            }
            set(Name{}, *static_cast<T*>(t), value);
            return true;
          }},
          get_(+[](Name, const void* t) -> Type {
            using poly::get;
            return get(Name{}, *static_cast<const T*>(t));
          }) {}
    constexpr property_entry() noexcept = default;
    constexpr bool set(Name, void* t, const Type& value) const noexcept {
      assert(set_);
      assert(t);
      return (*set_)(Name{}, t, value);
    }

    constexpr Type get(Name, const void* t) const noexcept {
      assert(get_);
      assert(t);
      return (*get_)(Name{}, t);
    }

    void (*set_)(Name, void*, const Type&) = nullptr;
    Type (*get_)(Name, const void*) = nullptr;
  };
  /// @}

  /// table of ptable entries
  template<POLY_PROP_SPEC... PropertySpec>
  struct property_table : public property_entry<PropertySpec>... {

    constexpr property_table() noexcept {}

    template<typename T>
    constexpr property_table(poly::traits::Id<T> id) noexcept
        : property_entry<PropertySpec>(id)... {}

    template<POLY_PROP_SPEC Spec>
    static property_offset_type property_offset(traits::Id<Spec>) noexcept {
      static_assert(
          (std::is_standard_layout_v<property_entry<PropertySpec>> && ...));
      constexpr property_table<PropertySpec...> t;
      const std::byte* this_ =
          static_cast<const std::byte*>(static_cast<const void*>(&t));
      const std::byte* entry_ = static_cast<const std::byte*>(
          static_cast<const void*>(static_cast<const property_entry<Spec>*>(&t)));
      return static_cast<property_offset_type>(entry_ - this_);
    }
  };

  template<typename T, POLY_PROP_SPEC... PropertySpecs>
  inline const property_table<PropertySpecs...> ptable_for =
      property_table<PropertySpecs...>(poly::traits::Id<T>{});
  /// @}

  /// returns the Spec in Specs belonging to Name
  template<typename Name, POLY_PROP_SPEC... Specs>
  struct spec_by_name {
    template<typename T>
    using predicate = std::is_same<property_name_t<T>, Name>;
    using list = filter_t<type_list<Specs...>, predicate>;
    static_assert(detail::list_size<list>::value == 1,
                  "No property with such name exists");
    using type = at_t<filter_t<type_list<Specs...>, predicate>, 0>;
  };

} // namespace detail
} // namespace poly
#endif
