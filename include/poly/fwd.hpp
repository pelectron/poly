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
#ifndef POLY_FWD_HPP
#define POLY_FWD_HPP
#include "poly/config.hpp"

#include "poly/traits.hpp"

namespace poly {

class ref_storage;

template<std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_storage;

template<std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class move_only_local_storage;

template<std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class sbo_storage;

template<std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class move_only_sbo_storage;

template<typename... Ts>
class variant_storage;
} // namespace poly
#endif
