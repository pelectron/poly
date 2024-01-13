#include "poly.hpp"
#include <catch2/catch_all.hpp>
POLY_METHOD(method);
struct method2{};
POLY_PROPERTY(property);
using OBJ = poly::Object<PROPERTIES(property(int)), METHODS(int(method), int(method2), int(method2, int)), 32>;
using REF = poly::Interface<PROPERTIES(property(int)), METHODS(int(method), int(method2), int(method2, int))>;
struct S1 {
  int property;
};

int extend(method, S1 &) { return 42; }
int extend(method2, S1 &) { return 54; }
int extend(method2, S1 &, int i) { return i+1; }

struct S2 {
  int *p;
};
int extend(method, S2 &) { return 43; }
int extend(method2, S2 &) { return 53; }
int extend(method2, S2 &, int i) { return i+2; }

int get(property, const S2 &) { return 5; }

void set(property, S2 &s, const int &i) { *(s.p) = i; }

TEMPLATE_TEST_CASE("generic interface test", "[interface]", REF, OBJ) {
  using If = TestType;
  S1 s1{79};
  int i = {77};
  S2 s2{&i};
  If interface(s1);
  SECTION("S1::method") {
    CHECK(interface.template call<method>() == 42);
    CHECK(interface.method() == 42);
  }
  SECTION("S1::method2") {
    CHECK(interface.template call<method2>() == 54);
    CHECK(interface.template call<method2>(41) == 42);
  }
  SECTION("S1::property") {
    CHECK(interface.property() == 79);
    interface.property(55);
    CHECK(interface.property() == 55);
    CHECK(interface.template get<property>() == 55);

    interface.template set<property>(22);
    CHECK(interface.template get<property>() == 22);
    CHECK(interface.property() == 22);
  }
  interface = s2;
  SECTION("S2::method") {
    CHECK(interface.template call<method>() == 43);
    CHECK(interface.method() == 43);
  }
  SECTION("S2::method2") {
    CHECK(interface.template call<method2>() == 53);
    CHECK(interface.template call<method2>(41) == 43);
  }
  SECTION("S2::property") {
    CHECK(i == 77);
    CHECK(interface.property() == 5);
    interface.property(59);
    CHECK(i == 59);
    CHECK(interface.property() == 5);
    CHECK(interface.template get<property>() == 5);

    interface.template set<property>(25);
    CHECK(i == 25);
    CHECK(interface.template get<property>() == 5);
    CHECK(interface.property() == 5);
  }
}
