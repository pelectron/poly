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
#include "poly/function.hpp"
#include "catch2/catch_all.hpp"
#include "poly/storage.hpp"
using Fn = poly::function<int(int) const,poly::local_storage<32, 8>>;
// using Fn2 = poly::any_function<poly::local_storage<32, 8>, int(int) const,
//                                int(float), float(double)>;

template <typename... Ts> struct overload : public Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts...) -> overload<std::decay_t<Ts>...>;

TEST_CASE("function") {
  Fn f{[](int i) -> int { return i - 1; }};
  REQUIRE(f(43) == 42);
  // Fn2 f2{overload{[](int i) -> int { return i - 1; },
  //                 [](float f) mutable -> int { return f + 1; },
  //                 [](double d) mutable -> float { return d + 2; }}};
  // REQUIRE(f2(43i32) == 42);
  // REQUIRE(f2(43.0f) == 44);
  // REQUIRE(f2(43.0) == 45.0f);
  // f.bind([](int i) -> int { return i + 1; });
  // REQUIRE(f(43) == 44);
}
