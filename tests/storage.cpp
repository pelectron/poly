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
#include "poly/storage.hpp"
#include "catch2/catch_test_macros.hpp"
#include <catch2/catch_all.hpp>
using namespace poly;

static_assert(poly::is_storage_v<local_storage<32, 8>>);
static_assert(poly::is_storage_v<local_storage<64, 4>>);
static_assert(poly::is_storage_v<move_only_local_storage<32, 8>>);
static_assert(poly::is_storage_v<move_only_local_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<local_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<local_storage<32, 8>>);
static_assert(std::is_move_constructible_v<local_storage<32, 8>>);
static_assert(std::is_move_assignable_v<local_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<move_only_local_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<move_only_local_storage<32, 8>>);
static_assert(std::is_move_constructible_v<move_only_local_storage<32, 8>>);
static_assert(std::is_move_assignable_v<move_only_local_storage<32, 8>>);

static_assert(poly::is_storage_v<sbo_storage<32, 8>>);
static_assert(poly::is_storage_v<sbo_storage<64, 4>>);
static_assert(poly::is_storage_v<move_only_sbo_storage<32, 8>>);
static_assert(poly::is_storage_v<move_only_sbo_storage<64, 4>>);

static_assert(std::is_copy_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_copy_assignable_v<sbo_storage<32, 8>>);
static_assert(std::is_move_constructible_v<sbo_storage<32, 8>>);
static_assert(std::is_move_assignable_v<sbo_storage<32, 8>>);

static_assert(not std::is_copy_constructible_v<move_only_sbo_storage<32, 8>>);
static_assert(not std::is_copy_assignable_v<move_only_sbo_storage<32, 8>>);
static_assert(std::is_move_constructible_v<move_only_sbo_storage<32, 8>>);
static_assert(std::is_move_assignable_v<move_only_sbo_storage<32, 8>>);

template <size_t Size, size_t Align> class Tracker {
public:
  Tracker(int &c) : count(&c) {
    assert(count != nullptr);
    ++(*count);
  }
  Tracker(const Tracker &o) : count(o.count) {
    assert(count != nullptr);
    ++(*count);
  }
  Tracker(Tracker &&o) : count(o.count) {
    ++(*count);
    assert(count != nullptr);
  }
  Tracker &operator=(const Tracker &o) {
    count = o.count;
    assert(count != nullptr);
    ++(*count);
    return *this;
  }
  Tracker &operator=(Tracker &&o) {
    count = o.count;
    assert(count != nullptr);
    ++(*count);
    return *this;
  }
  ~Tracker() {
    assert(count != nullptr);
    --(*count);
  }

private:
  int *count;
  alignas(Align) std::byte buffer[Size]{};
};
TEST_CASE("ref_storage", "[storage]") {
  int count = 0;
  Tracker<64, 8> a(count);
  REQUIRE(count == 1);
  {
    poly::ref_storage s;
    REQUIRE(s.data() == nullptr);
    s.emplace(a);
    REQUIRE(count == 1);
    REQUIRE(s.data() == &a);
    s.reset();
    REQUIRE(s.data() == nullptr);
  }
  REQUIRE(count == 1);
}

/// covers:
///   - construction from T
TEMPLATE_TEST_CASE(
    "storage ctor", "[storage]",
    (poly::type_list<poly::local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 16>>)) {

  using Storage = poly::at_t<TestType, 0>;
  // using Object = poly::at_t<TestType, 1>;
  SECTION("default constructed storages are empty") {
    Storage s;
    REQUIRE(s.data() == nullptr);
  }
}

/// covers:
///   - copy construction from storage
///   - copy assignment
TEMPLATE_TEST_CASE(
    "storage copy", "[storage]",
    (poly::type_list<poly::local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>),
    (poly::type_list<
        poly::variant_storage<Tracker<64, 16>, Tracker<64, 8>, Tracker<8, 16>>,
        Tracker<64, 16>>),
    (poly::type_list<
        poly::variant_storage<Tracker<64, 16>, Tracker<64, 8>, Tracker<8, 16>>,
        Tracker<64, 8>>),
    (poly::type_list<
        poly::variant_storage<Tracker<64, 16>, Tracker<64, 8>, Tracker<8, 16>>,
        Tracker<8, 16>>)) {

  using Storage = poly::at_t<TestType, 0>;
  using Object = poly::at_t<TestType, 1>;
  SECTION("ctor") {
    SECTION("copy construct from empty storage") {
      const Storage s1{};
      Storage s2{s1};
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("copy construct from non empty storage") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(Object{count});
      REQUIRE(count == 1);
      Storage s2(s1);
      REQUIRE(count == 2);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() != nullptr);
    }
  }
  SECTION("assignment") {
    SECTION("copy assign empty into empty") {
      const Storage s1{};
      Storage s2{};
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
      s2 = s1;
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("copy assign to self") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(Object{count});
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      s1 = s1;
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
    }
    SECTION("copy assign non empty into empty") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(Object{count});
      Storage s2{};
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() == nullptr);
      s2 = s1;
      REQUIRE(count == 2);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() != nullptr);
    }
    SECTION("copy assign empty into non empty") {
      int count = 0;
      const Storage s1{};
      Storage s2{};
      s2.template emplace<Object>(Object{count});
      REQUIRE(count == 1);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() != nullptr);
      s2 = s1;
      REQUIRE(count == 0);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("copy assign non empty into non empty") {
      int count = 0;
      int count2 = 0;
      Storage s1{};
      s1.template emplace<Object>(Object{count});
      Storage s2{};
      s2.template emplace<Object>(Object{count2});
      REQUIRE(count == 1);
      REQUIRE(count2 == 1);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() != nullptr);
      s2 = s1;
      REQUIRE(count == 2);
      REQUIRE(count2 == 0);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() != nullptr);
    }
  }
}
/// covers:
///   - move construction from storage
///   - move assignment
TEMPLATE_TEST_CASE(
    "storage move", "[storage]",
    (poly::type_list<poly::local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::local_storage<32, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::local_storage<32, 8>, Tracker<24, 4>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<24, 4>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 16>>)) {

  using Storage = poly::at_t<TestType, 0>;
  using Object = poly::at_t<TestType, 1>;
  SECTION("move") {
    SECTION("move construct from empty storage") {
      Storage s1{};
      Storage s2{std::move(s1)};
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("move construct from non empty storage") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(count);
      REQUIRE(count == 1);
      Storage s2{std::move(s1)};
      REQUIRE(count == 1);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() != nullptr);
    }
  }
  SECTION("assignment") {
    SECTION("move assign empty into empty") {
      Storage s1{};
      Storage s2{};
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
      s2 = std::move(s1);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("move assign to self") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(count);
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      s1 = std::move(s1);
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
    }
    SECTION("move assign non empty into empty") {
      int count = 0;
      Storage s1{};
      s1.template emplace<Object>(count);
      Storage s2{};
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() == nullptr);
      s2 = std::move(s1);
      REQUIRE(count == 1);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() != nullptr);
    }
    SECTION("move assign empty into non empty") {
      int count = 0;
      Storage s1{};
      Storage s2{};
      s2.template emplace<Object>(count);
      REQUIRE(count == 1);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() != nullptr);
      s2 = std::move(s1);
      REQUIRE(count == 0);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() == nullptr);
    }
    SECTION("move assign non empty into non empty") {
      int count = 0;
      int count2 = 0;
      Storage s1{};
      s1.template emplace<Object>(count);
      Storage s2{};
      s2.template emplace<Object>(count2);
      REQUIRE(count == 1);
      REQUIRE(count2 == 1);
      REQUIRE(s1.data() != nullptr);
      REQUIRE(s2.data() != nullptr);
      s2 = std::move(s1);
      REQUIRE(count == 1);
      REQUIRE(count2 == 0);
      REQUIRE(s1.data() == nullptr);
      REQUIRE(s2.data() != nullptr);
    }
  }
}
/// covers:
///   -storage dtor
TEMPLATE_TEST_CASE(
    "storage dtor", "[storage]",
    (poly::type_list<poly::local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 16>>)) {
  using Storage = poly::at_t<TestType, 0>;
  using Object = poly::at_t<TestType, 1>;
  int count = 0;
  {
    REQUIRE(count == 0);
    Storage s1{};
    s1.template emplace<Object>(count);
    REQUIRE(count == 1);
  }
  REQUIRE(count == 0);
}

TEMPLATE_TEST_CASE(
    "storage::emplace", "[storage]",
    (poly::type_list<poly::local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_local_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<8, 16>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::move_only_sbo_storage<32, 8>, Tracker<64, 16>>)) {
  using Storage = poly::at_t<TestType, 0>;
  using Object = poly::at_t<TestType, 1>;
  SECTION("emplace into empty storage") {
    int count = 0;
    Storage s{};
    REQUIRE(count == 0);
    s.template emplace<Object>(count);
    REQUIRE(count == 1);
    REQUIRE(s.data() != nullptr);
  }
  SECTION("emplace into non empty storage") {
    int count = 0;
    int count2 = 0;
    Storage s{};
    s.template emplace<Object>(count2);
    REQUIRE(s.data() != nullptr);
    REQUIRE(count == 0);
    REQUIRE(count2 == 1);
    s.template emplace<Object>(count);
    REQUIRE(count == 1);
    REQUIRE(count2 == 0);
    REQUIRE(s.data() != nullptr);
  }
}

/// covers:
///   - move assignment
TEMPLATE_TEST_CASE(
    "storage move assign", "[storage]",
    /* SBO storage */
    /* */
    /* begin: object fits into both buffers*/
    /* with same alignment and size*/
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<32, 8>,
                     Tracker<8, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<32, 8>,
                     Tracker<24, 8>, Tracker<8, 8>>),
    /* with same alignment and different size*/
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<32, 8>,
                     Tracker<8, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<64, 8>,
                     Tracker<8, 8>, Tracker<24, 8>>),
    /* with different alignment and same size*/
    (poly::type_list<poly::sbo_storage<32, 16>, poly::sbo_storage<32, 8>,
                     Tracker<8, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<32, 16>,
                     Tracker<24, 8>, Tracker<8, 8>>),
    /* end: object fits into both buffers*/

    /* begin: object fits into one buffer, but not other -> always different
       sizes*/
    /*storages have same alignment*/
    /* only fits one because of size*/

    /* second object fits*/
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<40, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<40, 8>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<24, 8>, Tracker<40, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<24, 8>, Tracker<40, 8>>),
    /* second object does not fit*/
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<40, 8>, Tracker<32, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<40, 8>, Tracker<32, 8>>),
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<32, 8>, Tracker<40, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<32, 8>, Tracker<40, 8>>),

    /* storages have different alignment */
    /* only fits one because of alignment */
    /* second object fits */
    (poly::type_list<poly::sbo_storage<64, 16>, poly::sbo_storage<24, 8>,
                     Tracker<40, 16>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<24, 16>, poly::sbo_storage<64, 8>,
                     Tracker<40, 16>, Tracker<24, 8>>),
    (poly::type_list<poly::sbo_storage<64, 16>, poly::sbo_storage<24, 8>,
                     Tracker<24, 8>, Tracker<40, 16>>),
    (poly::type_list<poly::sbo_storage<24, 16>, poly::sbo_storage<64, 8>,
                     Tracker<24, 8>, Tracker<40, 16>>),
    /* only fits one because of size and alignment*/
    (poly::type_list<poly::sbo_storage<64, 16>, poly::sbo_storage<24, 8>,
                     Tracker<40, 16>, Tracker<24, 16>>),
    (poly::type_list<poly::sbo_storage<24, 16>, poly::sbo_storage<64, 8>,
                     Tracker<40, 16>, Tracker<24, 16>>),
    (poly::type_list<poly::sbo_storage<64, 16>, poly::sbo_storage<24, 8>,
                     Tracker<24, 16>, Tracker<40, 16>>),
    (poly::type_list<poly::sbo_storage<24, 16>, poly::sbo_storage<64, 8>,
                     Tracker<24, 16>, Tracker<40, 16>>),

    /* end: object fits into one buffer, but not other -> always different
       sizes*/

    /* begin: object on heap*/
    /* storages have same alignment */
    /* on heap because of size*/
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<40, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<40, 8>, Tracker<64, 8>>),
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<64, 8>, Tracker<40, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<64, 8>, Tracker<40, 8>>),
    /* on heap because of alignment*/
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<24, 8>,
                     Tracker<40, 16>, Tracker<24, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<32, 8>,
                     Tracker<40, 16>, Tracker<24, 16>>),
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<24, 8>,
                     Tracker<24, 16>, Tracker<40, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<32, 8>,
                     Tracker<24, 16>, Tracker<40, 16>>),
    /* on heap because of size and alignment*/
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<128, 16>, Tracker<64, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<128, 16>, Tracker<64, 16>>),
    (poly::type_list<poly::sbo_storage<64, 8>, poly::sbo_storage<24, 8>,
                     Tracker<64, 16>, Tracker<128, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<64, 16>, Tracker<128, 16>>),
    /* storages have different alignment */
    /* on heap because of size*/
    (poly::type_list<poly::sbo_storage<32, 16>, poly::sbo_storage<24, 8>,
                     Tracker<40, 8>, Tracker<32, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<32, 16>,
                     Tracker<40, 8>, Tracker<32, 8>>),
    (poly::type_list<poly::sbo_storage<32, 16>, poly::sbo_storage<24, 8>,
                     Tracker<32, 8>, Tracker<40, 8>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<32, 16>,
                     Tracker<32, 8>, Tracker<40, 8>>),
    /* on heap because of alignment*/
    (poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<24, 4>,
                     Tracker<24, 16>, Tracker<24, 16>>),
    (poly::type_list<poly::sbo_storage<24, 4>, poly::sbo_storage<32, 8>,
                     Tracker<24, 16>, Tracker<24, 16>>),
    /* on heap because of size and alignment*/
    (poly::type_list<poly::sbo_storage<32, 16>, poly::sbo_storage<24, 8>,
                     Tracker<40, 16>, Tracker<64, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<32, 16>,
                     Tracker<40, 16>, Tracker<64, 16>>),
    (poly::type_list<poly::sbo_storage<32, 16>, poly::sbo_storage<24, 8>,
                     Tracker<64, 16>, Tracker<40, 16>>),
    (poly::type_list<poly::sbo_storage<24, 8>, poly::sbo_storage<64, 8>,
                     Tracker<64, 16>, Tracker<40, 16>>)
    /* end: object on heap*/) {

  using Storage1 = poly::at_t<TestType, 0>;
  using Storage2 = poly::at_t<TestType, 1>;
  using Object1 = poly::at_t<TestType, 2>;
  using Object2 = poly::at_t<TestType, 3>;
  SECTION("assign from empty into empty storage") {
    Storage1 s1{};
    Storage2 s2{};
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() == nullptr);
    s2 = std::move(s1);
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() == nullptr);
  }
  SECTION("assign to self") {
    int count = 0;
    Storage1 s1{};
    s1.template emplace<Object1>(count);
    REQUIRE(count == 1);
    REQUIRE(s1.data() != nullptr);
    s1 = std::move(s1);
    REQUIRE(count == 1);
    REQUIRE(s1.data() != nullptr);
  }
  SECTION("assign non empty into empty storage") {
    int count = 0;
    Storage1 s1{};
    s1.template emplace<Object1>(count);
    Storage2 s2{};
    REQUIRE(count == 1);
    REQUIRE(s1.data() != nullptr);
    REQUIRE(s2.data() == nullptr);
    s2 = std::move(s1);
    REQUIRE(count == 1);
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() != nullptr);
  }
  SECTION("assign empty into non empty storage") {
    int count = 0;
    Storage1 s1{};
    Storage1 s2{};
    s2.template emplace<Object2>(count);
    REQUIRE(count == 1);
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() != nullptr);
    s2 = std::move(s1);
    REQUIRE(count == 0);
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() == nullptr);
  }
  SECTION("assign non empty into non empty storage") {
    int count = 0;
    int count2 = 0;
    Storage1 s1{};
    s1.template emplace<Object1>(count);
    Storage1 s2{};
    s2.template emplace<Object2>(count2);
    REQUIRE(count == 1);
    REQUIRE(count2 == 1);
    REQUIRE(s1.data() != nullptr);
    REQUIRE(s2.data() != nullptr);
    s2 = std::move(s1);
    REQUIRE(count == 1);
    REQUIRE(count2 == 0);
    REQUIRE(s1.data() == nullptr);
    REQUIRE(s2.data() != nullptr);
  }
}
using A = poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<16, 8>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
using B = poly::type_list<poly::sbo_storage<16, 8>, poly::sbo_storage<32, 8>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
using C = poly::type_list<poly::sbo_storage<16, 4>, poly::sbo_storage<32, 8>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
using D = poly::type_list<poly::sbo_storage<32, 8>, poly::sbo_storage<16, 4>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
using E = poly::type_list<poly::sbo_storage<16, 8>, poly::sbo_storage<32, 4>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
using F = poly::type_list<poly::sbo_storage<32, 4>, poly::sbo_storage<16, 8>,
                          Tracker<8, 8>, Tracker<16, 8>, Tracker<32, 8>,
                          Tracker<8, 16>, Tracker<32, 16>>;
/// covers:
/// - copy ctor for different sizes
/// - copy assignment for different sizes
TEMPLATE_TEST_CASE("copy sbo storage of different sizes", "[storage]", A, B, C,
                   D, E, F) {

  using Storage1 = poly::at_t<TestType, 0>;
  using Storage2 = poly::at_t<TestType, 1>;
  using SmallObject =
      poly::at_t<TestType, 2>; // fits into buffer of both storages
  using MediumObject =
      poly::at_t<TestType, 3>; // fits into buffer of larger storage
  using LargeObject = poly::at_t<TestType, 4>; // only fits on heap
  using OverAlignedSmallObject =
      poly::at_t<TestType,
                 5>; // would fit into smaller storage if not for alignment
  using OverAlignedLargeObject =
      poly::at_t<TestType, 6>; // would not fit either buffer

  SECTION("copy Storage1 into Storage2") {
    SECTION("copy ctor") {
      SECTION("construct from empty storage") {
        const Storage1 bs{};
        Storage2 ss{bs};
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
      }
      SECTION("construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
      }
    }
    SECTION("copy assignment") {
      SECTION("copy assign empty into empty storage") {
        const Storage1 bs{};
        Storage2 ss{};
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
        ss = bs;
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
      }
      SECTION("copy assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
      }
      SECTION("copy assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          Storage2 ss{};
          ss.template emplace<MediumObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          Storage2 ss{};
          ss.template emplace<OverAlignedSmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          Storage2 ss{};
          ss.template emplace<OverAlignedLargeObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          Storage2 ss{};
          ss.template emplace<LargeObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("self assignment") {
          int count = 0;
          Storage1 s1;
          s1.template emplace<SmallObject>(count);
          REQUIRE(count == 1);
          s1 = s1;
          REQUIRE(count == 1);
        }
      }
    }
  }
}
using G = poly::type_list<poly::move_only_sbo_storage<32, 8>,
                          poly::move_only_sbo_storage<16, 8>, Tracker<8, 8>,
                          Tracker<16, 8>, Tracker<32, 8>, Tracker<8, 16>,
                          Tracker<32, 16>>;
using H = poly::type_list<poly::move_only_sbo_storage<16, 8>,
                          poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>,
                          Tracker<16, 8>, Tracker<32, 8>, Tracker<8, 16>,
                          Tracker<32, 16>>;
using I = poly::type_list<poly::move_only_sbo_storage<16, 4>,
                          poly::move_only_sbo_storage<32, 8>, Tracker<8, 8>,
                          Tracker<16, 8>, Tracker<32, 8>, Tracker<8, 16>,
                          Tracker<32, 16>>;
using J = poly::type_list<poly::move_only_sbo_storage<32, 8>,
                          poly::move_only_sbo_storage<16, 4>, Tracker<8, 8>,
                          Tracker<16, 8>, Tracker<32, 8>, Tracker<8, 16>,
                          Tracker<32, 16>>;
/// covers:
/// - move ctor for different sizes
/// - move assignment for different sizes
TEMPLATE_TEST_CASE("move sbo storage of different sizes", "[storage]", A, B, C,
                   D, E, F, G, H, I, J) {

  using Storage1 = poly::at_t<TestType, 0>;
  using Storage2 = poly::at_t<TestType, 1>;
  using SmallObject =
      poly::at_t<TestType, 2>; // fits into buffer of both storages
  using MediumObject =
      poly::at_t<TestType, 3>; // fits into buffer of larger storage
  using LargeObject = poly::at_t<TestType, 4>; // only fits on heap
  using OverAlignedSmallObject =
      poly::at_t<TestType,
                 5>; // would fit into smaller storage if not for alignment
  using OverAlignedLargeObject =
      poly::at_t<TestType, 6>; // would not fit either buffer

  SECTION("move Storage1 into Storage2") {
    SECTION("move ctor") {
      SECTION("construct from empty storage") {
        Storage1 bs{};
        Storage2 ss{std::move(bs)};
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
      }
      SECTION("construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
      }
    }
    SECTION("move assignment") {
      SECTION("move assign empty into empty storage") {
        Storage1 bs{};
        Storage2 ss{};
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
        ss = std::move(bs);
        REQUIRE(bs.data() == nullptr);
        REQUIRE(ss.data() == nullptr);
      }
      SECTION("move assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("self assignment") {
          int count = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          REQUIRE(count == 1);
          bs = std::move(bs);
          REQUIRE(count == 1);
        }
      }
      SECTION("move assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<SmallObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<MediumObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedSmallObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<OverAlignedLargeObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{};
          bs.template emplace<LargeObject>(count);
          Storage2 ss{};
          ss.template emplace<SmallObject>(count2);
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
      }
    }
  }
}
