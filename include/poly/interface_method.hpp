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
#ifndef POLY_INTERFACE_TABLE_HPP
#define POLY_INTERFACE_TABLE_HPP

#include "poly/object_method.hpp"

namespace poly::detail {
/// an entry in the InterfaceTable. It stores the offset of the corresponding
/// VTableEntry.
/// @{
template <POLY_METHOD_SPEC Spec> struct InterfaceVTableEntry;

template <typename Ret, typename Method, typename... Args>
struct InterfaceVTableEntry<Ret(Method, Args...)> {
  using signature_type = Ret(Method, Args...);

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
/// @}

} // namespace poly::detail
#endif
