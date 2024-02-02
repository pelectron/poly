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
#include <iostream>

POLY_METHOD(method);
POLY_METHOD(method2);
POLY_METHOD(method3);
POLY_METHOD(method4);
POLY_METHOD(method5);
POLY_METHOD(method6);
POLY_METHOD(method7);
POLY_METHOD(method8);
POLY_METHOD(method9);
struct m1 {};
struct m2 {};
struct m3 {};
struct m4 {};
struct m5 {};
struct m6 {};
struct m7 {};
struct m8 {};
struct m9 {};
POLY_PROPERTY(property1);
POLY_PROPERTY(property2);
POLY_PROPERTY(property3);
POLY_PROPERTY(property4);
POLY_PROPERTY(property5);
POLY_PROPERTY(property6);
POLY_PROPERTY(property7);
POLY_PROPERTY(property8);
POLY_PROPERTY(property9);
struct property11 {};
struct property12 {};
struct property13 {};
struct property14 {};
struct property15 {};
struct property16 {};
struct property17 {};
struct property18 {};
struct property19 {};
int main() {
  std::cout << "different poly::basic_object sizes in bytes" << std::endl;

  std::cout << "base size: "
            << sizeof(poly::Reference<poly::type_list<>, poly::type_list<>>)
            << std::endl;

  std::cout << "One named property: "
            << sizeof(poly::Reference<poly::type_list<property1(int)>,
                                      poly::type_list<>>)
            << std::endl;

  std::cout << "One unnamed property: "
            << sizeof(poly::Reference<poly::type_list<property11(int)>,
                                      poly::type_list<>>)
            << std::endl;

  std::cout
      << "9 named properties: "
      << sizeof(poly::Reference<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<>>)
      << std::endl;

  std::cout << "9 unnamed properties: "
            << sizeof(poly::Reference<
                      poly::type_list<
                          property11(int), property12(int), property13(int),
                          property14(int), property15(int), property16(int),
                          property17(int), property18(int), property19(int)>,
                      poly::type_list<>>)
            << std::endl;

  std::cout
      << "1 named method: "
      << sizeof(
             poly::Reference<poly::type_list<>, poly::type_list<void(method)>>)
      << std::endl;
  std::cout
      << "1 unnamed method: "
      << sizeof(poly::Reference<poly::type_list<>, poly::type_list<void(m1)>>)
      << std::endl;
  std::cout
      << "9 named methods: "
      << sizeof(poly::Reference<
                poly::type_list<>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout
      << "9 unnamed methods: "
      << sizeof(poly::Reference<poly::type_list<>,
                                poly::type_list<void(m1), void(m2), void(m3),
                                                void(m4), void(m5), void(m6),
                                                void(m7), void(m8), void(m9)>>)
      << std::endl;

  std::cout << "1 named method, 1 named property: "
            << sizeof(poly::Reference<poly::type_list<property1(int)>,
                                      poly::type_list<void(method)>>)
            << std::endl;
  std::cout << "1 unnamed method, 1 named property: "
            << sizeof(poly::Reference<poly::type_list<property1(int)>,
                                      poly::type_list<void(m1)>>)
            << std::endl;
  std::cout << "1 named method, 1 unnamed property: "
            << sizeof(poly::Reference<poly::type_list<property11(int)>,
                                      poly::type_list<void(method)>>)
            << std::endl;
  std::cout << "1 unnamed method, 1 unnamed property: "
            << sizeof(poly::Reference<poly::type_list<property11(int)>,
                                      poly::type_list<void(m1)>>)
            << std::endl;

  std::cout
      << "9 named methods, 9 named properties: "
      << sizeof(poly::Reference<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout
      << "9 unnamed methods, 9 named properties: "
      << sizeof(poly::Reference<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<void(m1), void(m2), void(m3), void(m4),
                                void(m5), void(m6), void(m7), void(m8),
                                void(m9)>>)
      << std::endl;
  std::cout
      << "9 named methods, 9 unnamed properties: "
      << sizeof(poly::Reference<
                poly::type_list<
                    property11(int), property12(int), property13(int),
                    property14(int), property15(int), property16(int),
                    property17(int), property18(int), property19(int)>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout << "9 unnamed methods, 9 unnamed properties: "
            << sizeof(poly::Reference<
                      poly::type_list<
                          property11(int), property12(int), property13(int),
                          property14(int), property15(int), property16(int),
                          property17(int), property18(int), property19(int)>,
                      poly::type_list<void(m1), void(m2), void(m3), void(m4),
                                      void(m5), void(m6), void(m7), void(m8),
                                      void(m9)>>)
            << std::endl;
  std::cout << "different poly::basic_interface sizes in bytes" << std::endl;

  std::cout << "base size: "
            << sizeof(poly::Interface<poly::type_list<>, poly::type_list<>>)
            << std::endl;

  std::cout << "One named property: "
            << sizeof(poly::Interface<poly::type_list<property1(int)>,
                                      poly::type_list<>>)
            << std::endl;

  std::cout << "One unnamed property: "
            << sizeof(poly::Interface<poly::type_list<property11(int)>,
                                      poly::type_list<>>)
            << std::endl;

  std::cout
      << "9 named properties: "
      << sizeof(poly::Interface<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<>>)
      << std::endl;

  std::cout << "9 unnamed properties: "
            << sizeof(poly::Interface<
                      poly::type_list<
                          property11(int), property12(int), property13(int),
                          property14(int), property15(int), property16(int),
                          property17(int), property18(int), property19(int)>,
                      poly::type_list<>>)
            << std::endl;

  std::cout
      << "1 named method: "
      << sizeof(
             poly::Interface<poly::type_list<>, poly::type_list<void(method)>>)
      << std::endl;
  std::cout
      << "1 unnamed method: "
      << sizeof(poly::Interface<poly::type_list<>, poly::type_list<void(m1)>>)
      << std::endl;
  std::cout
      << "9 named methods: "
      << sizeof(poly::Interface<
                poly::type_list<>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout
      << "9 unnamed methods: "
      << sizeof(poly::Interface<poly::type_list<>,
                                poly::type_list<void(m1), void(m2), void(m3),
                                                void(m4), void(m5), void(m6),
                                                void(m7), void(m8), void(m9)>>)
      << std::endl;

  std::cout << "1 named method, 1 named property: "
            << sizeof(poly::Interface<poly::type_list<property1(int)>,
                                      poly::type_list<void(method)>>)
            << std::endl;
  std::cout << "1 unnamed method, 1 named property: "
            << sizeof(poly::Interface<poly::type_list<property1(int)>,
                                      poly::type_list<void(m1)>>)
            << std::endl;
  std::cout << "1 named method, 1 unnamed property: "
            << sizeof(poly::Interface<poly::type_list<property11(int)>,
                                      poly::type_list<void(method)>>)
            << std::endl;
  std::cout << "1 unnamed method, 1 unnamed property: "
            << sizeof(poly::Interface<poly::type_list<property11(int)>,
                                      poly::type_list<void(m1)>>)
            << std::endl;

  std::cout
      << "9 named methods, 9 named properties: "
      << sizeof(poly::Interface<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout
      << "9 unnamed methods, 9 named properties: "
      << sizeof(poly::Interface<
                poly::type_list<property1(int), property2(int), property3(int),
                                property4(int), property5(int), property6(int),
                                property7(int), property8(int), property9(int)>,
                poly::type_list<void(m1), void(m2), void(m3), void(m4),
                                void(m5), void(m6), void(m7), void(m8),
                                void(m9)>>)
      << std::endl;
  std::cout
      << "9 named methods, 9 unnamed properties: "
      << sizeof(poly::Interface<
                poly::type_list<
                    property11(int), property12(int), property13(int),
                    property14(int), property15(int), property16(int),
                    property17(int), property18(int), property19(int)>,
                poly::type_list<void(method), void(method2), void(method3),
                                void(method4), void(method5), void(method6),
                                void(method7), void(method8), void(method9)>>)
      << std::endl;
  std::cout << "9 unnamed methods, 9 unnamed properties: "
            << sizeof(poly::Interface<
                      poly::type_list<
                          property11(int), property12(int), property13(int),
                          property14(int), property15(int), property16(int),
                          property17(int), property18(int), property19(int)>,
                      poly::type_list<void(m1), void(m2), void(m3), void(m4),
                                      void(m5), void(m6), void(m7), void(m8),
                                      void(m9)>>)
            << std::endl;
}
