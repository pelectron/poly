#include "poly/traits.hpp"
#include <catch2/catch_all.hpp>
struct property1{};

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
  CHECK(std::is_same_v<poly::value_type_t<const property1(TestType)>, TestType>);
  CHECK(std::is_same_v<poly::value_type_t<property1(TestType) noexcept>, TestType>);
  CHECK(std::is_same_v<poly::value_type_t<const property1(TestType) noexcept>,
                       TestType>);
}
TEMPLATE_TEST_CASE("property_name_t", "[traits]", int, float, size_t) {
  CHECK(std::is_same_v<poly::property_name_t<property1(TestType)>, property1>);
  CHECK(std::is_same_v<poly::property_name_t<const property1(TestType)>, property1>);
  CHECK(std::is_same_v<poly::property_name_t<property1(TestType) noexcept>, property1>);
  CHECK(std::is_same_v<poly::property_name_t<const property1(TestType) noexcept>,
                       property1>);
}
