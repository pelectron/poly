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
#include <catch2/catch_all.hpp>

POLY_METHOD(method);
POLY_METHOD(method2);

POLY_PROPERTY(property);
struct property2 {};

using OBJ =
    poly::Struct<poly::sbo_storage<32>,
                 POLY_PROPERTIES(property(int), property2(float)),
                 POLY_METHODS(int(method), int(method2), int(method2, int),
                              int(method2, int, float),
                              int(method2, int, double), void(method2, float))>;

using Interface =
    poly::InterfaceRef<POLY_PROPERTIES(property(int), property2(float)),
                       POLY_METHODS(int(method), int(method2, int),
                                    int(method2))>;
using Interface2 =
    poly::InterfaceRef<POLY_PROPERTIES(property2(float), property(int)),
                       POLY_METHODS(int(method2), int(method),
                                    int(method2, int))>;
using REF =
    poly::Reference<POLY_PROPERTIES(property(int), property2(float)),
                    POLY_METHODS(int(method), int(method2), int(method2, int),
                                 int(method2, int, float),
                                 int(method2, int, double))>;

struct S1 {
  int property;
  float property2;
  char data_[128];
};
struct X {
  void (*f)();
};

int extend(method, S1&) { return 42; }
int extend(method2, S1&) { return 54; }
int extend(method2, S1&, int i) { return i + 1; }
void extend(method2, S1&, float) {}
int extend(method2, S1&, int i, float) { return i - 1; }
int extend(method2, S1&, int i, double) { return i - 2; }

float get(property2, const S1& s) { return s.property2; }

void set(property2, S1& s, const float& f) { s.property2 = f; }
bool check(property2, const S1&, float f) { return f < 100.0f; }

struct S2 {
  int* p;
  float* f;
};
int extend(method, S2&) { return 43; }
int extend(method2, S2&) { return 53; }
int extend(method2, S2&, int i) { return i + 2; }
void extend(method2, S2&, float) { return; }
int extend(method2, S2&, int i, float) { return i; }
int extend(method2, S2&, int i, double) { return i - 1; }

int get(property, const S2&) { return 5; }

void set(property, S2& s, const int& i) { *(s.p) = i; }

float get(property2, const S2&s) { return *(s.f); }

void set(property2, S2& s, const float& f) { *(s.f) = f + 1; }
TEMPLATE_TEST_CASE("generic interface test", "[interface]", OBJ) {
  using If = TestType;
  S1 s1{79, 9.0f, {}};
  int i = {77};
  float f = 10.0f;
  S2 s2{&i, &f};
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

    REQUIRE(object.template set<property>(22));
    REQUIRE(object.template get<property>() == 22);
    REQUIRE(object.property == 22);

    REQUIRE(object.template get<property2>() == 9.0f);
    REQUIRE(object.template set<property2>(15.0f));
    REQUIRE(object.template get<property2>() == 15.0f);
    REQUIRE_FALSE(object.template set<property2>(100.1f));
    REQUIRE(object.template get<property2>() == 15.0f);
  }
  SECTION("Interface S1 methods") {
    SECTION("same order of methods") {
      Interface sub_interface{object};
      REQUIRE(sub_interface.template call<method>() == 42);
      REQUIRE(sub_interface.method() == 42);
      // int()
      REQUIRE(sub_interface.template call<method2>() == 54);
      REQUIRE(sub_interface.method2() == 54);
      // int(int)
      REQUIRE(sub_interface.template call<method2>(41) == 42);
      REQUIRE(sub_interface.method2(41) == 42);
    }
    SECTION("different order of methods") {
      Interface2 sub_interface{object};
      REQUIRE(sub_interface.template call<method>() == 42);
      REQUIRE(sub_interface.method() == 42);
      // int()
      REQUIRE(sub_interface.template call<method2>() == 54);
      REQUIRE(sub_interface.method2() == 54);
      // int(int)
      REQUIRE(sub_interface.template call<method2>(41) == 42);
      REQUIRE(sub_interface.method2(41) == 42);
    }
  }
  SECTION("Interface S1 property") {
    SECTION("same order of properties") {
      Interface sub_interface{object};
      REQUIRE(sub_interface.property == 79);
      sub_interface.property = 55;
      REQUIRE(sub_interface.property == 55);
      REQUIRE(sub_interface.template get<property>() == 55);

      REQUIRE(sub_interface.template set<property>(22));
      REQUIRE(sub_interface.template get<property>() == 22);
      REQUIRE(sub_interface.property == 22);

      REQUIRE(sub_interface.template get<property2>() == 9.0f);
      REQUIRE(sub_interface.template set<property2>(15.0f));
      REQUIRE(sub_interface.template get<property2>() == 15.0f);
      REQUIRE_FALSE(sub_interface.template set<property2>(100.1f));
      REQUIRE(sub_interface.template get<property2>() == 15.0f);
    }
    SECTION("different order of properties") {
      Interface2 sub_interface{object};
      REQUIRE(sub_interface.property == 79);
      sub_interface.property = 55;
      REQUIRE(sub_interface.property == 55);
      REQUIRE(sub_interface.template get<property>() == 55);

      REQUIRE(sub_interface.template set<property>(22));
      REQUIRE(sub_interface.template get<property>() == 22);
      REQUIRE(sub_interface.property == 22);

      REQUIRE(sub_interface.template get<property2>() == 9.0f);
      REQUIRE(sub_interface.template set<property2>(15.0f));
      REQUIRE(sub_interface.template get<property2>() == 15.0f);
      REQUIRE_FALSE(sub_interface.template set<property2>(100.1f));
      REQUIRE(sub_interface.template get<property2>() == 15.0f);
    }
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
  SECTION("Interface S2 methods") {
    SECTION("same order of methods") {
      Interface sub_interface{object};
      REQUIRE(sub_interface.template call<method>() == 43);
      REQUIRE(sub_interface.method() == 43);
      // int()
      REQUIRE(sub_interface.template call<method2>() == 53);
      REQUIRE(sub_interface.method2() == 53);
      // int(int)
      REQUIRE(sub_interface.template call<method2>(41) == 43);
      REQUIRE(sub_interface.method2(41) == 43);
    }
    SECTION("different order of methods") {
      Interface2 sub_interface{object};
      REQUIRE(sub_interface.template call<method>() == 43);
      REQUIRE(sub_interface.method() == 43);
      // int()
      REQUIRE(sub_interface.template call<method2>() == 53);
      REQUIRE(sub_interface.method2() == 53);
      // int(int)
      REQUIRE(sub_interface.template call<method2>(41) == 43);
      REQUIRE(sub_interface.method2(41) == 43);
    }
  }
  SECTION("Interface S2 property") {
    SECTION("same order of properties") {
      Interface sub_interface{object};
      REQUIRE(sub_interface.property == 5);
      sub_interface.property = 55;
      REQUIRE(*s2.p == 55);
      REQUIRE(sub_interface.property == 5);
      REQUIRE(sub_interface.template get<property>() == 5);
      REQUIRE(*s2.p == 55);

      REQUIRE(sub_interface.template set<property>(22));
      REQUIRE(*s2.p == 22);
      REQUIRE(sub_interface.template get<property>() == 5);
      REQUIRE(sub_interface.property == 5);

      REQUIRE(sub_interface.template get<property2>() == 10.0f);
      REQUIRE(sub_interface.template set<property2>(15.0f));
      REQUIRE(sub_interface.template get<property2>() == 16.0f);
      REQUIRE(sub_interface.template set<property2>(100.1f));
      REQUIRE(sub_interface.template get<property2>() == 101.1f);
    }
    SECTION("different order of properties") {
      Interface2 sub_interface{object};
      REQUIRE(sub_interface.property == 5);
      sub_interface.property = 55;
      REQUIRE(*s2.p == 55);
      REQUIRE(sub_interface.property == 5);
      REQUIRE(sub_interface.template get<property>() == 5);
      REQUIRE(*s2.p == 55);

      REQUIRE(sub_interface.template set<property>(22));
      REQUIRE(*s2.p == 22);
      REQUIRE(sub_interface.template get<property>() == 5);
      REQUIRE(sub_interface.property == 5);

      REQUIRE(sub_interface.template get<property2>() == 10.0f);
      REQUIRE(sub_interface.template set<property2>(15.0f));
      REQUIRE(sub_interface.template get<property2>() == 16.0f);
      REQUIRE(sub_interface.template set<property2>(100.1f));
      REQUIRE(sub_interface.template get<property2>() == 101.1f);
    }
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
