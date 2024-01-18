#include "poly.hpp"
#include <catch2/catch_all.hpp>

POLY_METHOD(method);
POLY_METHOD(method2);

POLY_PROPERTY(property);
class OBJ : public poly::basic_interface<
                poly::local_storage<32>, PROPERTIES(property(int)),
                METHODS(int(method), int(method2), int(method2, int),
                        int(method2, int, float), int(method2, int, double))> {
public:
  using Base =
      poly::basic_interface<poly::local_storage<32>, properties, methods>;
  using Base::Base;
  using Base::operator=;
  POLY_USE_OVERLOAD(method2, int());
  POLY_USE_OVERLOAD(method2, int(int));
  POLY_USE_OVERLOAD(method2, int(int, float));
  POLY_USE_OVERLOAD(method2, int(int, double));
};

using REF = poly::Interface<PROPERTIES(property(int)),
                            METHODS(int(method), int(method2),
                                    int(method2, int), int(method2, int, float),
                                    int(method2, int, double))>;
struct S1 {
  int property;
};

int extend(method, S1 &) { return 42; }
int extend(method2, S1 &) { return 54; }
int extend(method2, S1 &, int i) { return i + 1; }
int extend(method2, S1 &, int i, float) { return i - 1; }
int extend(method2, S1 &, int i, double) { return i - 2; }

struct S2 {
  int *p;
};
int extend(method, S2 &) { return 43; }
int extend(method2, S2 &) { return 53; }
int extend(method2, S2 &, int i) { return i + 2; }
int extend(method2, S2 &, int i, float) { return i; }
int extend(method2, S2 &, int i, double) { return i - 1; }

int get(property, const S2 &) { return 5; }

void set(property, S2 &s, const int &i) { *(s.p) = i; }

TEMPLATE_TEST_CASE("generic interface test", "[interface]", OBJ) {
  using If = TestType;
  S1 s1{79};
  int i = {77};
  S2 s2{&i};
  If interface(s1);
  SECTION("S1::method") {
    REQUIRE(interface.template call<method>() == 42);
    REQUIRE(interface.method() == 42);
  }
  SECTION("S1::method2") {
    // int()
    REQUIRE(interface.template call<method2>() == 54);
    REQUIRE(interface.method2() == 54);
    // int(int)
    REQUIRE(interface.template call<method2>(41) == 42);
    REQUIRE(interface.method2(41) == 42);
    // int(int, float)
    REQUIRE(interface.template call<method2>(41, 2.0f) == 40);
    REQUIRE(interface.method2(41, 2.0f) == 40);
    // int(int, double)
    REQUIRE(interface.template call<method2>(41, 2.0) == 39);
    REQUIRE(interface.method2(41, 2.0) == 39);
  }
  SECTION("S1::property") {
    REQUIRE(interface.property() == 79);
    interface.property(55);
    REQUIRE(interface.property() == 55);
    REQUIRE(interface.template get<property>() == 55);

    interface.template set<property>(22);
    REQUIRE(interface.template get<property>() == 22);
    REQUIRE(interface.property() == 22);
  }
  interface = s2;
  SECTION("S2::method2") {
    // int()
    REQUIRE(interface.template call<method2>() == 53);
    REQUIRE(interface.method2() == 53);
    // int(int)
    REQUIRE(interface.template call<method2>(41) == 43);
    REQUIRE(interface.method2(41) == 43);
    // int(int, float)
    REQUIRE(interface.template call<method2>(41, 2.0f) == 41);
    REQUIRE(interface.method2(41, 2.0f) == 41);
    // int(int, double)
    REQUIRE(interface.template call<method2>(41, 2.0) == 40);
    REQUIRE(interface.method2(41, 2.0) == 40);
  }
  SECTION("S2::property") {
    REQUIRE(i == 77);
    REQUIRE(interface.property() == 5);
    interface.property(59);
    REQUIRE(i == 59);
    REQUIRE(interface.property() == 5);
    REQUIRE(interface.template get<property>() == 5);

    interface.template set<property>(25);
    REQUIRE(i == 25);
    REQUIRE(interface.template get<property>() == 5);
    REQUIRE(interface.property() == 5);
  }
}
