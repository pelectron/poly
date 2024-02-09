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
#include "poly.hpp"
#include <cstddef>
#include <iostream>

template<typename Self, typename T>
struct POLY_EMPTY_BASE member {
  T get() const { return T{}; }
};
template<typename Self>
struct POLY_EMPTY_BASE S2 : member<Self, int> {
  S2() : member<Self, int>{}, mf{} {}

  POLY_NO_UNIQUE_ADDRESS member<Self, float> mf;
};
template<typename Self>
struct POLY_EMPTY_BASE S3 : S2<Self> {
  S3() : S2<Self>{}, mf{} {}

  POLY_NO_UNIQUE_ADDRESS member<Self, double> mf;
};
struct POLY_EMPTY_BASE S4 : S3<S4> {};

template<typename... T>
struct S5 : member<S5<T...>, T>... {};
template<typename... T>
struct POLY_EMPTY_BASE S6 : member<S6<T...>, T>... {};

static constexpr size_t size4 = sizeof(S4);
static constexpr size_t size5 = sizeof(S5<int, float, double>);
static constexpr size_t size6 = sizeof(S6<int, float, double>);
int main() {
  std::cout << size4 << std::endl;
  std::cout << size5 << std::endl;
  std::cout << size6 << std::endl;
}
