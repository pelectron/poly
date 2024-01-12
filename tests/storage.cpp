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
  volatile int *count;
};
TEST_CASE("ref_storage", "[storage]") {
  tracker a;
  CHECK(tracker::alive == 1);
  {
    poly::ref_storage s;
    CHECK(s.data() == nullptr);
    s.emplace(a);
    CHECK(tracker::alive == 1);
    CHECK(s.data() == &a);
  }
  CHECK(tracker::alive == 1);
}
TEMPLATE_TEST_CASE("generic copyable storage test", "[storage]",
                   (poly::local_storage<32, 8>), (poly::sbo_storage<32, 8>)) {
  using Storage = TestType;

  SECTION("default constructed storages are empty") {
    Storage s;
    CHECK(s.data() == nullptr);
  }
  SECTION("ctor from T") {
    SECTION("move a T") {
      int count = 0;
      Storage s{std::move(Track{count})};
      CHECK(count == 1);
      CHECK(s.data() != nullptr);
    }
    SECTION("copy a T") {
      int count = 0;
      const Track t{count};
      CHECK(count == 1);
      Storage s{t};
      CHECK(count == 2);
      CHECK(s.data() != nullptr);
    }
  }
  SECTION("copy ctor") {
    SECTION("copy empty storage") {
      const Storage s1{};
      Storage s2{s1};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("copy non empty storage") {
      int count = 0;
      const Storage s1{Track{count}};
      CHECK(count == 1);
      Storage s2(s1); // TODO: FIX
      CHECK(count == 2);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("move ctor") {
    SECTION("move empty storage") {
      Storage s1{};
      Storage s2{std::move(s1)};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("move non empty storage") {
      int count = 0;
      Storage s1{Track{count}};
      CHECK(count == 1);
      Storage s2{std::move(s1)};
      CHECK(count == 1);
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("copy assignment") {
    SECTION("copy assign empty storage") {
      const Storage s1{};
      Storage s2{};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
      s2 = s1;
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("copy assign non empty storage") {
      int count = 0;
      const Storage s1{Track(count)};
      Storage s2{};
      CHECK(count == 1);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() == nullptr);
      s2 = s1; // TODO: FIX
      CHECK(count == 2);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("move assignment") {
    SECTION("move assign empty storage") {
      Storage s1{};
      Storage s2{};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
      s2 = std::move(s1);
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("move assign non empty storage") {
      int count = 0;
      Storage s1{Track(count)};
      Storage s2{};
      CHECK(count == 1);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() == nullptr);
      s2 = std::move(s1);
      CHECK(count == 1);
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("emplace") {
    SECTION("emplace into empty storage") {
      int count = 0;
      Storage s{};
      CHECK(count == 0);
      s.template emplace<Track>(count);
      CHECK(count == 1);
      CHECK(s.data() != nullptr);
    }
    SECTION("emplace into non empty storage") {
      int count = 0;
      int count2 = 0;
      Storage s{Track{count2}};
      CHECK(s.data() != nullptr);
      CHECK(count == 0);
      CHECK(count2 == 1);
      s.template emplace<Track>(count);
      CHECK(count == 1);
      CHECK(count2 == 0);
      CHECK(s.data() != nullptr);
    }
  }
  SECTION("dtor") {
    int count = 0;
    {
      CHECK(count == 0);
      Storage s1{Track{count}};
      CHECK(count == 1);
    }
    CHECK(count == 0);
  }
}
