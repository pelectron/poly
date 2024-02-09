/**
 * Copyright 2024 Pelé Constam
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file poly/storage.hpp
 * This file provides several storage types.
 *
 * - ref_storage: a non owning storage type.
 * - (move_only_)local_storage: storage without dynamic allocation
 * - (move_only_)sbo_storage: storage with small buffer optimization
 * - (move_only_)heap_storage: storage which always allocates on the heap.
 *   Always stores values on the heap.
 * - variant_storage: stores any of the types provided as its template
 *   arguments.
 */
#ifndef POLY_STRORAGE_HPP
#define POLY_STRORAGE_HPP

#include "poly/storage/heap_storage.hpp"
#include "poly/storage/local_storage.hpp"
#include "poly/storage/ref_storage.hpp"
#include "poly/storage/sbo_storage.hpp"
#include "poly/storage/variant_storage.hpp"

#endif
