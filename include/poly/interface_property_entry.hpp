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
#ifndef POLY_INTERFACE_PROPERTY_HPP
#define POLY_INTERFACE_PROPERTY_HPP
#include "poly/property_table.hpp"
namespace poly::detail {
/// @addtogroup ptable
/// @{

/// An individual entry in the interface property table. It stores to offset
/// of the corresponding property_entry in the original property_table.
/// @{
template<POLY_PROP_SPEC PropertySpec>
struct interface_property_entry;
template<typename Name, typename Type>
struct interface_property_entry<const Name(Type)> {
  Type get(Name, const void* table, const void* t) const {
    assert(table);
    assert(t);
    const auto* entry = static_cast<const property_entry<const Name(Type)>*>(
        static_cast<const void*>(static_cast<const std::byte*>(table) +
                                 offset));
    return entry->get(Name{}, t);
  }

  property_offset_type offset{0};
};

template<typename Name, typename Type>
struct interface_property_entry<const Name(Type) noexcept> {

  Type get(Name, const void* table, const void* t) const noexcept {
    assert(table);
    assert(t);
    const auto* entry =
        static_cast<const property_entry<const Name(Type) noexcept>*>(
            static_cast<const void*>(static_cast<const std::byte*>(table) +
                                     offset));
    return entry->get(Name{}, t);
  }

  property_offset_type offset{0};
};

template<typename Name, typename Type>
struct interface_property_entry<Name(Type)> {
  bool set(Name, const void* table, void* t, const Type& value) const {
    assert(table);
    assert(t);
    const auto* entry =
        static_cast<const property_entry<Name(Type)>*>(static_cast<const void*>(
            static_cast<const std::byte*>(table) + offset));
    return entry->set(Name{}, t, value);
  }

  Type get(Name, const void* table, const void* t) const {
    assert(table);
    assert(t);
    const auto* entry =
        static_cast<const property_entry<Name(Type)>*>(static_cast<const void*>(
            static_cast<const std::byte*>(table) + offset));
    return entry->get(Name{}, t);
  }

  property_offset_type offset{0};
};

template<typename Name, typename Type>
struct interface_property_entry<Name(Type) noexcept> {
  bool set(Name, const void* table, void* t, const Type& value) const noexcept {
    assert(table);
    assert(t);
    const auto* entry = static_cast<const property_entry<Name(Type) noexcept>*>(
        static_cast<const void*>(static_cast<const std::byte*>(table) +
                                 offset));
    return entry->set(Name{}, t, value);
  }

  Type get(Name, const void* table, const void* t) const {
    assert(table);
    assert(t);
    const auto* entry = static_cast<const property_entry<Name(Type) noexcept>*>(
        static_cast<const void*>(static_cast<const std::byte*>(table) +
                                 offset));
    return entry->get(Name{}, t);
  }

  property_offset_type offset{0};
};

/// @}

} // namespace poly::detail
#endif
