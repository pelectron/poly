#include "poly/storage.hpp"
#include "catch2/catch_test_macros.hpp"
#include <atomic>
#include <catch2/catch_all.hpp>

struct tracker {
  tracker() { ++alive; }
  tracker(const tracker &) { ++alive; }
  tracker &operator=(tracker &&) {
    ++alive;
    return *this;
  }
  tracker &operator=(const tracker &) {
    ++alive;
    return *this;
  }
  tracker(tracker &&) { ++alive; }
  ~tracker() { --alive; }
  static std::atomic<int> alive;
  int data{0};
};

std::atomic<int> tracker::alive = 0;

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
struct Track {

  Track(int &c) : count(&c) {
    assert(count != nullptr);
    ++(*count);
  }
  Track(const Track &o) : count(o.count) {
    assert(count != nullptr);
    ++(*count);
  }
  Track(Track &&o) : count(o.count) {
    ++(*count);
    assert(count != nullptr);
  }
  Track &operator=(const Track &o) {
    count = o.count;
    assert(count != nullptr);
    ++(*count);
    return *this;
  }
  Track &operator=(Track &&o) {
    count = o.count;
    assert(count != nullptr);
    ++(*count);
    return *this;
  }
  ~Track() {
    assert(count != nullptr);
    --(*count);
  }
  int *count;
};
struct MediumTrack : Track {

  MediumTrack(int &c) : Track(c) {}
  MediumTrack(const MediumTrack &o) : Track(o) {}
  MediumTrack(MediumTrack &&o) : Track(std::move(o)) {}
  MediumTrack &operator=(const MediumTrack &o) {
    Track::operator=(o);
    return *this;
  }
  MediumTrack &operator=(MediumTrack &&o) {
    Track::operator=(std::move(o));
    return *this;
  }
  ~MediumTrack() = default;

private:
  std::byte data_[64]{};
};
struct MediumTrack2 : Track {

  MediumTrack2(int &c) : Track(c) {}
  MediumTrack2(const MediumTrack2 &o) : Track(o) {}
  MediumTrack2(MediumTrack2 &&o) : Track(std::move(o)) {}
  MediumTrack2 &operator=(const MediumTrack2 &o) {
    Track::operator=(o);
    return *this;
  }
  MediumTrack2 &operator=(MediumTrack2 &&o) {
    Track::operator=(std::move(o));
    return *this;
  }
  ~MediumTrack2() = default;

private:
  alignas(64) std::byte data_[32]{};
};
struct BigTrack : Track {

  BigTrack(int &c) : Track(c) {}
  BigTrack(const BigTrack &o) : Track(o) {}
  BigTrack(BigTrack &&o) : Track(std::move(o)) {}
  BigTrack &operator=(const BigTrack &o) {
    Track::operator=(o);
    return *this;
  }
  BigTrack &operator=(BigTrack &&o) {
    Track::operator=(std::move(o));
    return *this;
  }
  ~BigTrack() = default;

private:
  std::byte data_[256]{};
};
TEST_CASE("ref_storage", "[storage]") {
  tracker a;
  REQUIRE(tracker::alive == 1);
  {
    poly::ref_storage s;
    REQUIRE(s.data() == nullptr);
    s.emplace(a);
    REQUIRE(tracker::alive == 1);
    REQUIRE(s.data() == &a);
    s.reset();
    REQUIRE(s.data() == nullptr);
  }
  REQUIRE(tracker::alive == 1);
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
  using Object = poly::at_t<TestType, 1>;
  SECTION("default constructed storages are empty") {
    Storage s;
    REQUIRE(s.data() == nullptr);
  }
  SECTION("ctor from T") {
    SECTION("move a T") {
      int count = 0;
      Storage s{std::move(Object{count})};
      REQUIRE(count == 1);
      REQUIRE(s.data() != nullptr);
    }
    SECTION("copy a T") {
      int count = 0;
      const Object t{count};
      REQUIRE(count == 1);
      Storage s{t};
      REQUIRE(count == 2);
      REQUIRE(s.data() != nullptr);
    }
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
    (poly::type_list<poly::sbo_storage<32, 8>, Tracker<64, 16>>)) {

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
      const Storage s1{Object{count}};
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
      Storage s1{Object(count)};
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      s1 = s1;
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
    }
    SECTION("copy assign non empty into empty") {
      int count = 0;
      const Storage s1{Object(count)};
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
      Storage s2{Object(count)};
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
      const Storage s1{Object(count)};
      Storage s2{Object(count2)};
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
      Storage s1{Object{count}};
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
      Storage s1{Object(count)};
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
      s1 = std::move(s1);
      REQUIRE(count == 1);
      REQUIRE(s1.data() != nullptr);
    }
    SECTION("move assign non empty into empty") {
      int count = 0;
      Storage s1{Object(count)};
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
      Storage s2{Object(count)};
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
      Storage s1{Object(count)};
      Storage s2{Object(count2)};
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
    Storage s1{Object{count}};
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
    Storage s{Object{count2}};
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
    Storage1 s1{Object1(count)};
    REQUIRE(count == 1);
    REQUIRE(s1.data() != nullptr);
    s1 = std::move(s1);
    REQUIRE(count == 1);
    REQUIRE(s1.data() != nullptr);
  }
  SECTION("assign non empty into empty storage") {
    int count = 0;
    Storage1 s1{Object1(count)};
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
    Storage2 s2{Object1(count)};
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
    Storage1 s1{Object1(count)};
    Storage2 s2{Object2(count2)};
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
/// covers:
/// - copy ctor for different sizes
/// - copy assignment for different sizes
TEMPLATE_TEST_CASE("copy sbo storage of different sizes", "[storage]", A, B,
                   C) {

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
          const Storage1 bs{SmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const Storage1 bs{MediumObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          const Storage1 bs{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          const Storage1 bs{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{bs};
          REQUIRE(count == 2);
        }
        SECTION("object on heap") {
          int count = 0;
          const Storage1 bs{LargeObject(count)};
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
          const Storage1 bs{SmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const Storage1 bs{MediumObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          const Storage1 bs{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          const Storage1 bs{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("object on heap") {
          int count = 0;
          const Storage1 bs{LargeObject(count)};
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
          const Storage1 bs{SmallObject(count)};
          Storage2 ss{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          const Storage1 bs{MediumObject(count)};
          Storage2 ss{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          const Storage1 bs{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          const Storage1 bs{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = bs;
          REQUIRE(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          const Storage1 bs{LargeObject(count)};
          Storage2 ss{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = bs;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
      }
    }
  }
  SECTION("copy Storage2 into Storage1") {
    SECTION("copy ctor") {
      SECTION("copy construct from empty storage") {
        const Storage2 ss{};
        Storage1 bs{ss};
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
      }
      SECTION("copy construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const Storage2 ss{SmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{ss};
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const Storage2 ss{MediumObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{ss};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          const Storage2 ss{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{ss};
          REQUIRE(count == 2);
        }
        SECTION("overaligned object stays on heap") {
          int count = 0;
          const Storage2 ss{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{ss};
          REQUIRE(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const Storage2 ss{LargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{ss};
          REQUIRE(count == 2);
        }
      }
    }
    SECTION("copy assignment") {
      SECTION("copy assign empty into empty storage") {
        const Storage2 ss{};
        Storage1 bs{};
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
        bs = ss;
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
      }
      SECTION("copy assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const Storage2 ss{SmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = ss;
          REQUIRE(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const Storage2 ss{MediumObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = ss;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          const Storage2 ss{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = ss;
          REQUIRE(count == 2);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          const Storage2 ss{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = ss;
          REQUIRE(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const Storage2 ss{LargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = ss;
          REQUIRE(count == 2);
        }
      }
      SECTION("copy assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          const Storage2 ss{SmallObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = ss;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          const Storage2 ss{MediumObject(count)};
          Storage1 bs{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = ss;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          const Storage2 ss{OverAlignedSmallObject(count)};
          Storage1 bs{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = ss;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          int count2 = 0;
          const Storage2 ss{OverAlignedLargeObject(count)};
          Storage1 bs{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = ss;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          const Storage2 ss{LargeObject(count)};
          Storage1 bs{Track(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = ss;
          REQUIRE(count == 2);
          REQUIRE(count2 == 0);
        }
        SECTION("self assignment") {
          int count = 0;
          Storage2 ss{LargeObject(count)};
          REQUIRE(count == 1);
          ss = ss;
          REQUIRE(count == 1);
        }
      }
    }
  }
}
/// covers:
/// - move ctor for different sizes
/// - move assignment for different sizes
TEMPLATE_TEST_CASE("move sbo storage of different sizes", "[storage]", A, B,
                   C) {

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
          Storage1 bs{SmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{MediumObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{LargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{std::move(bs)};
          REQUIRE(count == 1);
        }
      }
    }
    SECTION("move assignment") {
      SECTION("move assign empty into empty storage") {
        const Storage1 bs{};
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
          Storage1 bs{SmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage1 bs{MediumObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage1 bs{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage1 bs{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
        SECTION("object on heap") {
          int count = 0;
          Storage1 bs{LargeObject(count)};
          REQUIRE(count == 1);
          Storage2 ss{};
          ss = std::move(bs);
          REQUIRE(count == 1);
        }
      }
      SECTION("move assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{SmallObject(count)};
          Storage2 ss{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{MediumObject(count)};
          Storage2 ss{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{OverAlignedSmallObject(count)};
          Storage2 ss{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{OverAlignedLargeObject(count)};
          Storage2 ss{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          Storage1 bs{LargeObject(count)};
          Storage2 ss{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          ss = std::move(bs);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
        }
      }
    }
  }
  SECTION("move Storage2 into Storage1") {
    SECTION("move ctor") {
      SECTION("move construct from empty storage") {
        Storage2 ss{};
        Storage1 bs{std::move(ss)};
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
      }
      SECTION("move construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage2 ss{SmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{std::move(ss)};
          REQUIRE(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage2 ss{MediumObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{std::move(ss)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage2 ss{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{std::move(ss)};
          REQUIRE(count == 1);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          Storage2 ss{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{std::move(ss)};
          REQUIRE(count == 1);
        }
        SECTION("object stays on heap") {
          int count = 0;
          Storage2 ss{LargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{std::move(ss)};
          REQUIRE(count == 1);
        }
      }
    }
    SECTION("move assignment") {
      SECTION("move assign empty into empty storage") {
        Storage2 ss{};
        Storage1 bs{};
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
        bs = std::move(ss);
        REQUIRE(ss.data() == nullptr);
        REQUIRE(bs.data() == nullptr);
      }
      SECTION("move assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          Storage2 ss{SmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          Storage2 ss{MediumObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          Storage2 ss{OverAlignedSmallObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("overaligned object stays on heap") {
          int count = 0;
          Storage2 ss{OverAlignedLargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("object stays on heap") {
          int count = 0;
          Storage2 ss{LargeObject(count)};
          REQUIRE(count == 1);
          Storage1 bs{};
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(ss.data() == nullptr);
        }
      }
      SECTION("move assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          Storage2 ss{SmallObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage2 ss{MediumObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("overaligned object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          Storage2 ss{OverAlignedSmallObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("overaligned object on heap") {
          int count = 0;
          int count2 = 0;
          Storage2 ss{OverAlignedLargeObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
          REQUIRE(ss.data() == nullptr);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          Storage2 ss{OverAlignedLargeObject(count)};
          Storage1 bs{SmallObject(count2)};
          REQUIRE(count == 1);
          REQUIRE(count2 == 1);
          bs = std::move(ss);
          REQUIRE(count == 1);
          REQUIRE(count2 == 0);
          REQUIRE(ss.data() == nullptr);
        }
      }
    }
  }
}
