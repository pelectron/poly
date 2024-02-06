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
#include <catch2/catch_all.hpp>
struct property1 {};

TEMPLATE_TEST_CASE("is_property_spec_v", "[traits]", int, float, size_t) {
  CHECK(poly::is_property_spec_v<property1(TestType)>);
  CHECK(poly::is_property_spec_v<const property1(TestType)>);
  CHECK(poly::is_property_spec_v<property1(TestType) noexcept>);
  CHECK(poly::is_property_spec_v<const property1(TestType) noexcept>);
}
TEMPLATE_TEST_CASE("is_const_property_v", "[traits]", int, float, size_t) {
  CHECK_FALSE(poly::is_const_property_v<property1(TestType)>);
  CHECK(poly::is_const_property_v<const property1(TestType)>);
  CHECK_FALSE(poly::is_const_property_v<property1(TestType) noexcept>);
  CHECK(poly::is_const_property_v<const property1(TestType) noexcept>);
}
TEMPLATE_TEST_CASE("is_nothrow_property_v", "[traits]", int, float, size_t) {
  CHECK_FALSE(poly::is_nothrow_property_v<property1(TestType)>);
  CHECK_FALSE(poly::is_nothrow_property_v<const property1(TestType)>);
  CHECK(poly::is_nothrow_property_v<property1(TestType) noexcept>);
  CHECK(poly::is_nothrow_property_v<const property1(TestType) noexcept>);
}
TEMPLATE_TEST_CASE("value_type_t", "[traits]", int, float, size_t) {
  CHECK(std::is_same_v<poly::value_type_t<property1(TestType)>, TestType>);
  CHECK(
      std::is_same_v<poly::value_type_t<const property1(TestType)>, TestType>);
  CHECK(std::is_same_v<poly::value_type_t<property1(TestType) noexcept>,
                       TestType>);
  CHECK(std::is_same_v<poly::value_type_t<const property1(TestType) noexcept>,
                       TestType>);
}
TEMPLATE_TEST_CASE("property_name_t", "[traits]", int, float, size_t) {
  CHECK(std::is_same_v<poly::property_name_t<property1(TestType)>, property1>);
  CHECK(std::is_same_v<poly::property_name_t<const property1(TestType)>,
                       property1>);
  CHECK(std::is_same_v<poly::property_name_t<property1(TestType) noexcept>,
                       property1>);
  CHECK(
      std::is_same_v<poly::property_name_t<const property1(TestType) noexcept>,
                     property1>);
}
