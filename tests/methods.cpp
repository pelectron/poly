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
#include "poly/traits.hpp"
#include "poly/method.hpp"
#include <catch2/catch_all.hpp>
struct method1{};
POLY_METHOD(m1);

TEMPLATE_TEST_CASE("is_method_spec_v", "[traits]", int, float,size_t) {
  CHECK(poly::is_method_spec_v<TestType(method1)>);
  CHECK(poly::is_method_spec_v<TestType(method1) const>);
  CHECK(poly::is_method_spec_v<TestType(method1) noexcept>);
  CHECK(poly::is_method_spec_v<TestType(method1) const noexcept>);
  CHECK(poly::is_method_spec_v<TestType(method1, int)>);
  CHECK(poly::is_method_spec_v<TestType(method1, int) const>);
  CHECK(poly::is_method_spec_v<TestType(method1, int) noexcept>);
  CHECK(poly::is_method_spec_v<TestType(method1, int) const noexcept>);
}
TEMPLATE_TEST_CASE("is_const_method_v", "[traits]", int, float,size_t) {
  CHECK_FALSE(poly::is_const_method_v<TestType(method1)>);
  CHECK(poly::is_const_method_v<TestType(method1) const>);
  CHECK_FALSE(poly::is_const_method_v<TestType(method1) noexcept>);
  CHECK(poly::is_const_method_v<TestType(method1) const noexcept>);
  CHECK_FALSE(poly::is_const_method_v<TestType(method1, int)>);
  CHECK(poly::is_const_method_v<TestType(method1, int) const>);
  CHECK_FALSE(poly::is_const_method_v<TestType(method1, int) noexcept>);
  CHECK(poly::is_const_method_v<TestType(method1, int) const noexcept>);
}
TEMPLATE_TEST_CASE("is_nothrow_method_v", "[traits]", int, float,size_t) {
  CHECK_FALSE(poly::is_nothrow_method_v<TestType(method1)>);
  CHECK_FALSE(poly::is_nothrow_method_v<TestType(method1) const>);
  CHECK(poly::is_nothrow_method_v<TestType(method1) noexcept>);
  CHECK(poly::is_nothrow_method_v<TestType(method1) const noexcept>);
  CHECK_FALSE(poly::is_nothrow_method_v<TestType(method1, int)>);
  CHECK_FALSE(poly::is_nothrow_method_v<TestType(method1, int) const>);
  CHECK(poly::is_nothrow_method_v<TestType(method1, int) noexcept>);
  CHECK(poly::is_nothrow_method_v<TestType(method1, int) const noexcept>);
}
TEMPLATE_TEST_CASE("return_type_t", "[traits]", int, float,size_t) {
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1)>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1) const>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1) noexcept>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1) const noexcept>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1, int)>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1, int) const>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1, int) noexcept>>);
  CHECK(std::is_same_v<TestType, poly::return_type_t<TestType(method1, int) const noexcept>>);
}
TEMPLATE_TEST_CASE("method_name_t", "[traits]", int, float,size_t) {
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1)>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1) const>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1) noexcept>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1) const noexcept>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1, int)>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1, int) const>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1, int) noexcept>>);
  CHECK(std::is_same_v<method1, poly::method_name_t<TestType(method1, int) const noexcept>>);
}
