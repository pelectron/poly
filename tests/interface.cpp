#include "poly.hpp"
#include <catch2/catch_all.hpp>

POLY_METHOD(method);
POLY_METHOD(method2);

POLY_PROPERTY(property);
using OBJ =
    poly::basic_object<poly::sbo_storage<32>, PROPERTIES(property(int)),
                       METHODS(int(method), int(method2), int(method2, int),
                               int(method2, int, float),
                               int(method2, int, double),
                               void(method2, float))>;

using SubIf =
    poly::Interface<PROPERTIES(property(int)),
                    METHODS(int(method), int(method2, int),int(method2))>;

using REF = poly::Interface<PROPERTIES(property(int)),
                            METHODS(int(method), int(method2),
                                    int(method2, int), int(method2, int, float),
                                    int(method2, int, double))>;

struct S1 {
  int property;
  char data_[128];
};

int extend(method, S1 &) { return 42; }
int extend(method2, S1 &) { return 54; }
int extend(method2, S1 &, int i) { return i + 1; }
void extend(method2, S1 &, float f) {}
int extend(method2, S1 &, int i, float) { return i - 1; }
int extend(method2, S1 &, int i, double) { return i - 2; }

struct S2 {
  int *p;
};
int extend(method, S2 &) { return 43; }
int extend(method2, S2 &) { return 53; }
int extend(method2, S2 &, int i) { return i + 2; }
void extend(method2, S2 &, float f) { return; }
int extend(method2, S2 &, int i, float) { return i; }
int extend(method2, S2 &, int i, double) { return i - 1; }

int get(property, const S2 &) { return 5; }

void set(property, S2 &s, const int &i) { *(s.p) = i; }

TEMPLATE_TEST_CASE("generic interface test", "[interface]", OBJ) {
  using If = TestType;
  S1 s1{79};
  int i = {77};
  S2 s2{&i};
  If object(s1);
  SECTION("S1::method") {
    REQUIRE(object.template call<method>() == 42);
    REQUIRE(object.method() == 42);
  }
  SECTION("S1::method2") {
    // int()
    REQUIRE(object.template call<method2>() == 54);
    REQUIRE(object.method2() == 54);
    // int(int)
    REQUIRE(object.template call<method2>(41) == 42);
    REQUIRE(object.method2(41) == 42);
    // int(int, float)
    REQUIRE(object.template call<method2>(41, 2.0f) == 40);
    REQUIRE(object.method2(41, 2.0f) == 40);
    // int(int, double)
    REQUIRE(object.template call<method2>(41, 2.0) == 39);
    REQUIRE(object.method2(41, 2.0) == 39);
    // interface.method2(2.0f);
  }
  SECTION("S1::property") {
    REQUIRE(object.property == 79);
    object.property = 55;
    REQUIRE(object.property == 55);
    REQUIRE(object.template get<property>() == 55);

    object.template set<property>(22);
    REQUIRE(object.template get<property>() == 22);
    REQUIRE(object.property == 22);
  }
  SECTION("sub interface S1 methods") {
    SubIf sub_interface{object};
    REQUIRE(sub_interface.template call<method>() == 42);
    REQUIRE(sub_interface.method() == 42);
    // int()
    REQUIRE(sub_interface.template call<method2>() == 54);
    REQUIRE(sub_interface.method2() == 54);
    // int(int)
    REQUIRE(sub_interface.template call<method2>(41) == 42);
    REQUIRE(sub_interface.method2(41) == 42);
  }
  object = s2;
  SECTION("S2::method") {
    REQUIRE(object.template call<method>() == 43);
    REQUIRE(object.method() == 43);
  }
  SECTION("S2::method2") {
    // int()
    REQUIRE(object.template call<method2>() == 53);
    REQUIRE(object.method2() == 53);
    // int(int)
    REQUIRE(object.template call<method2>(41) == 43);
    REQUIRE(object.method2(41) == 43);
    // int(int, float)
    REQUIRE(object.template call<method2>(41, 2.0f) == 41);
    REQUIRE(object.method2(41, 2.0f) == 41);
    // int(int, double)
    REQUIRE(object.template call<method2>(41, 2.0) == 40);
    REQUIRE(object.method2(41, 2.0) == 40);
  }
  SECTION("sub interface S2 methods") {
    SubIf sub_interface{object};
    // int method()
    REQUIRE(sub_interface.template call<method>() == 43);
    REQUIRE(sub_interface.method() == 43);
    // int method2()
    REQUIRE(sub_interface.template call<method2>() == 53);
    REQUIRE(sub_interface.method2() == 53);
    // int method2(int)
    REQUIRE(sub_interface.template call<method2>(41) == 43);
    REQUIRE(sub_interface.method2(41) == 43);
  }
  SECTION("S2::property") {
    REQUIRE(i == 77);
    REQUIRE(object.property == 5);

    object.property = 59;
    REQUIRE(i == 59);
    REQUIRE(object.property == 5);
    REQUIRE(object.template get<property>() == 5);

    object.template set<property>(25);
    REQUIRE(i == 25);
    REQUIRE(object.template get<property>() == 5);
    REQUIRE(object.property == 5);
  }
}
