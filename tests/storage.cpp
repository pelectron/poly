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

  Track(int &c) : count(&c) { ++*count; }
  Track(const Track &other) : count(other.count) { ++*count; }
  Track(Track &&o) : count(o.count) { ++*count; }
  Track &operator=(const Track &o) {
    count = o.count;
    ++*count;
    return *this;
  }
  Track &operator=(Track &&o) {
    count = o.count;
    ++*count;
    return *this;
  }
  ~Track() { --*count; }
  int *count;
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
TEMPLATE_TEST_CASE_SIG("local_storage", "[storage]",
                       ((size_t S, size_t A), S, A), (32, 8), (32, 8)) {
  int count = 0;
  Track t(count);
  poly::local_storage<S, A> s;
  CHECK(count == 1);
  SECTION("reset") {
    CHECK(s.data() == nullptr);
    s.reset();
    CHECK(s.data() == nullptr);
  }
  CHECK(count == 1);
  SECTION("emplace and reset") {
    s.template emplace<Track>(count);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    s.reset();
    CHECK(count == 1);
    CHECK(s.data() == nullptr);
  }
  CHECK(count == 1);
  SECTION("ctor and dtor") {
    poly::local_storage<S, A> s1(t);
    CHECK(count == 2);
    CHECK(s1.data() != nullptr);
    poly::local_storage<S, A> s2(Track{count});
    CHECK(count == 3);
    CHECK(s2.data() != nullptr);
  }
  CHECK(count == 1);
  SECTION("local_storage operator=(const local_storage&)") {
    poly::local_storage<S, A> s1(t);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    CHECK(s1.data() != nullptr);
    s = s1;
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    s = poly::local_storage<S, A>(t);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
  }
}
TEMPLATE_TEST_CASE_SIG("local_move_only_storage", "[storage]",
                       ((size_t S, size_t A), S, A), (32, 8), (32, 8)) {
  int count = 0;
  Track t(count);
  poly::local_move_only_storage<S, A> s;
  CHECK(count == 1);
  SECTION("reset") {
    CHECK(s.data() == nullptr);
    s.reset();
    CHECK(s.data() == nullptr);
  }
  CHECK(count == 1);
  SECTION("emplace and reset") {
    s.template emplace<Track>(count);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    s.reset();
    CHECK(count == 1);
    CHECK(s.data() == nullptr);
  }
  CHECK(count == 1);
  SECTION("ctor and dtor") {
    poly::local_move_only_storage<S, A> s1(t);
    CHECK(count == 2);
    CHECK(s1.data() != nullptr);
    poly::local_move_only_storage<S, A> s2(Track{count});
    CHECK(count == 3);
    CHECK(s2.data() != nullptr);
  }
  CHECK(count == 1);
  SECTION("local_storage operator=(const local_storage&)") {
    poly::local_move_only_storage<S, A> s1(t);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    CHECK(s1.data() != nullptr);
    s = poly::local_move_only_storage<S, A>(t);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
    s = std::move(s1);
    CHECK(count == 2);
    CHECK(s.data() != nullptr);
  }
}
