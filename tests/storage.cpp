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
/// covers:
///   - copy construction
///   - copy assignment
TEMPLATE_TEST_CASE("generic copyable storage test", "[storage]",
                   (poly::local_storage<32, 8>), (poly::sbo_storage<32, 8>)) {

  using Storage = TestType;
  SECTION("copy ctor") {
    SECTION("copy construct from empty storage") {
      const Storage s1{};
      Storage s2{s1};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("copy construct from non empty storage") {
      int count = 0;
      const Storage s1{Track{count}};
      CHECK(count == 1);
      Storage s2(s1);
      CHECK(count == 2);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("copy assignment") {
    SECTION("copy assign empty into empty") {
      const Storage s1{};
      Storage s2{};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
      s2 = s1;
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("copy assign non empty into empty") {
      int count = 0;
      const Storage s1{Track(count)};
      Storage s2{};
      CHECK(count == 1);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() == nullptr);
      s2 = s1;
      CHECK(count == 2);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
    }
    SECTION("copy assign non empty into non empty") {
      int count = 0;
      int count2 = 0;
      const Storage s1{Track(count)};
      Storage s2{Track(count2)};
      CHECK(count == 1);
      CHECK(count2 == 1);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
      s2 = s1;
      CHECK(count == 2);
      CHECK(count2 == 0);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
}

/// covers:
///   - default constructor
///   - construction from T
///   - move construction
///   - move assignment
///   - emplace
///   - destructor
TEMPLATE_TEST_CASE("generic moveable storage test", "[storage]",
                   (poly::local_storage<32, 8>), (poly::sbo_storage<32, 8>),
                   (poly::local_move_only_storage<32, 8>),
                   (poly::sbo_move_only_storage<32, 8>)) {
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
  SECTION("move ctor") {
    SECTION("move construct from empty storage") {
      Storage s1{};
      Storage s2{std::move(s1)};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("move construct from non empty storage") {
      int count = 0;
      Storage s1{Track{count}};
      CHECK(count == 1);
      Storage s2{std::move(s1)};
      CHECK(count == 1);
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() != nullptr);
    }
  }
  SECTION("move assignment") {
    SECTION("move assign from empty into empty storage") {
      Storage s1{};
      Storage s2{};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
      s2 = std::move(s1);
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("move assign non empty into empty storage") {
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
    SECTION("move assign non empty into non empty storage") {
      int count = 0;
      int count2 = 0;
      Storage s1{Track(count)};
      Storage s2{Track(count2)};
      CHECK(count == 1);
      CHECK(count2 == 1);
      CHECK(s1.data() != nullptr);
      CHECK(s2.data() != nullptr);
      s2 = std::move(s1);
      CHECK(count == 1);
      CHECK(count2 == 0);
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
/// covers:
///   - ctor from big and small T
///   - move ctor
///   - move assignment
///   - dtor
TEMPLATE_TEST_CASE("moveable sbo storage test", "[storage]",
                   (poly::sbo_storage<32, 8>),
                   (poly::sbo_move_only_storage<32, 8>)) {

  using Storage = TestType;
  SECTION("ctor from T") {
    SECTION("move ctor with small T") {
      int count = 0;
      Storage s{std::move(Track{count})};
      CHECK(count == 1);
      CHECK(s.data() != nullptr);
    }
    SECTION("copy ctor with small T") {
      int count = 0;
      const Track t{count};
      CHECK(count == 1);
      Storage s{t};
      CHECK(count == 2);
      CHECK(s.data() != nullptr);
    }
    SECTION("move ctor with big T") {
      int count = 0;
      Storage s{std::move(BigTrack{count})};
      CHECK(count == 1);
      CHECK(s.data() != nullptr);
    }
    SECTION("copy ctor with big T") {
      int count = 0;
      const BigTrack t{count};
      CHECK(count == 1);
      Storage s{t};
      CHECK(count == 2);
      CHECK(s.data() != nullptr);
    }
  }
  SECTION("move ctor") {
    SECTION("move construct from empty storage") {
      Storage s1{};
      Storage s2{std::move(s1)};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("move construct from non empty storage") {
      SECTION("small object") {
        int count = 0;
        Storage s1{Track{count}};
        CHECK(count == 1);
        Storage s2(std::move(s1));
        CHECK(count == 1);
        CHECK(s1.data() == nullptr);
        CHECK(s2.data() != nullptr);
      }
      SECTION("big object") {
        int count = 0;
        Storage s1{BigTrack{count}};
        CHECK(count == 1);
        Storage s2(std::move(s1));
        CHECK(count == 1);
        CHECK(s1.data() == nullptr);
        CHECK(s2.data() != nullptr);
      }
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
      SECTION("small object") {
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
      SECTION("big object") {
        int count = 0;
        Storage s1{BigTrack(count)};
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
  }
  SECTION("dtor") {
    int count = 0;
    SECTION("containing small object") {
      CHECK(count == 0);
      Storage s1{Track{count}};
      CHECK(count == 1);
    }
    CHECK(count == 0);
    SECTION("containing small object") {
      {
        CHECK(count == 0);
        Storage s1{BigTrack{count}};
        CHECK(count == 1);
      }
      CHECK(count == 0);
    }
  }
}

/// covers:
///   - copy ctor
///   - copy assignment
TEMPLATE_TEST_CASE("copyable sbo storage test", "[storage]",
                   (poly::sbo_storage<32, 8>)) {
  using Storage = TestType;
  SECTION("copy ctor") {
    SECTION("copy construct from empty storage") {
      const Storage s1{};
      Storage s2{s1};
      CHECK(s1.data() == nullptr);
      CHECK(s2.data() == nullptr);
    }
    SECTION("copy construct from non empty storage") {
      SECTION("small object") {
        int count = 0;
        const Storage s1{Track{count}};
        CHECK(count == 1);
        Storage s2(s1);
        CHECK(count == 2);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() != nullptr);
      }
      SECTION("big object") {
        int count = 0;
        const Storage s1{BigTrack{count}};
        CHECK(count == 1);
        Storage s2(s1);
        CHECK(count == 2);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() != nullptr);
      }
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
      SECTION("small object") {
        int count = 0;
        const Storage s1{Track(count)};
        Storage s2{};
        CHECK(count == 1);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() == nullptr);
        s2 = s1;
        CHECK(count == 2);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() != nullptr);
      }
      SECTION("big object") {
        int count = 0;
        const Storage s1{BigTrack(count)};
        Storage s2{};
        CHECK(count == 1);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() == nullptr);
        s2 = s1;
        CHECK(count == 2);
        CHECK(s1.data() != nullptr);
        CHECK(s2.data() != nullptr);
      }
    }
  }
}
/// covers:
/// - copy ctor for different sizes
/// - copy assignment for different sizes
TEMPLATE_TEST_CASE("copy sbo storage of different sizes", "[storage]",
                   (poly::sbo_storage<32, 8>), (poly::sbo_storage<16, 8>)) {
  using BigStorage = poly::sbo_storage<128, 8>;
  using SmallStorage = TestType;

  SECTION("copy big into small buffer") {
    SECTION("copy ctor") {
      SECTION("copy construct from empty storage") {
        const BigStorage bs{};
        SmallStorage ss{bs};
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
      }
      SECTION("copy construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const BigStorage bs{Track(count)};
          CHECK(count == 1);
          SmallStorage ss{bs};
          CHECK(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const BigStorage bs{MediumTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{bs};
          CHECK(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const BigStorage bs{BigTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{bs};
          CHECK(count == 2);
        }
      }
    }
    SECTION("copy assignment") {
      SECTION("copy assign empty into empty storage") {
        const BigStorage bs{};
        SmallStorage ss{};
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
        ss = bs;
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
      }
      SECTION("copy assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const BigStorage bs{Track(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = bs;
          CHECK(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const BigStorage bs{MediumTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = bs;
          CHECK(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const BigStorage bs{BigTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = bs;
          CHECK(count == 2);
        }
      }
      SECTION("copy assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          const BigStorage bs{Track(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = bs;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          const BigStorage bs{MediumTrack(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = bs;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          const BigStorage bs{BigTrack(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = bs;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
      }
    }
  }
  SECTION("copy small into big buffer") {
    SECTION("copy ctor") {
      SECTION("copy construct from empty storage") {
        const SmallStorage ss{};
        BigStorage bs{ss};
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
      }
      SECTION("copy construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const SmallStorage ss{Track(count)};
          CHECK(count == 1);
          BigStorage bs{ss};
          CHECK(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const SmallStorage ss{MediumTrack(count)};
          CHECK(count == 1);
          BigStorage bs{ss};
          CHECK(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const SmallStorage ss{BigTrack(count)};
          CHECK(count == 1);
          BigStorage bs{ss};
          CHECK(count == 2);
        }
      }
    }
    SECTION("copy assignment") {
      SECTION("copy assign empty into empty storage") {
        const SmallStorage ss{};
        BigStorage bs{};
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
        bs = ss;
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
      }
      SECTION("copy assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          const SmallStorage ss{Track(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = ss;
          CHECK(count == 2);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          const SmallStorage ss{MediumTrack(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = ss;
          CHECK(count == 2);
        }
        SECTION("object stays on heap") {
          int count = 0;
          const SmallStorage ss{BigTrack(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = ss;
          CHECK(count == 2);
        }
      }
      SECTION("copy assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          const SmallStorage ss{Track(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = ss;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          const SmallStorage ss{MediumTrack(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = ss;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          const SmallStorage ss{BigTrack(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = ss;
          CHECK(count == 2);
          CHECK(count2 == 0);
        }
      }
    }
  }
}
/// covers:
/// - move ctor for different sizes
/// - move assignment for different sizes
TEMPLATE_TEST_CASE("move sbo storage of different sizes", "[storage]",
                   (poly::sbo_move_only_storage<32, 8>),
                   (poly::sbo_move_only_storage<16, 8>)) {
  using BigStorage = poly::sbo_move_only_storage<128, 8>;
  using SmallStorage = TestType;

  SECTION("move big into small storage") {
    SECTION("move ctor") {
      SECTION("move construct from empty storage") {
        BigStorage bs{};
        SmallStorage ss{std::move(bs)};
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
      }
      SECTION("move construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          BigStorage bs{Track(count)};
          CHECK(count == 1);
          SmallStorage ss{std::move(bs)};
          CHECK(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          BigStorage bs{MediumTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{std::move(bs)};
          CHECK(count == 1);
        }
        SECTION("object stays on heap") {
          int count = 0;
          BigStorage bs{BigTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{std::move(bs)};
          CHECK(count == 1);
        }
      }
    }
    SECTION("move assignment") {
      SECTION("move assign empty into empty storage") {
        BigStorage bs{};
        SmallStorage ss{};
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
        ss = std::move(bs);
        CHECK(bs.data() == nullptr);
        CHECK(ss.data() == nullptr);
      }
      SECTION("move assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          BigStorage bs{Track(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = std::move(bs);
          CHECK(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          BigStorage bs{MediumTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = std::move(bs);
          CHECK(count == 1);
        }
        SECTION("object stays on heap") {
          int count = 0;
          BigStorage bs{BigTrack(count)};
          CHECK(count == 1);
          SmallStorage ss{};
          ss = std::move(bs);
          CHECK(count == 1);
        }
      }
      SECTION("move assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          BigStorage bs{Track(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = std::move(bs);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          BigStorage bs{MediumTrack(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = std::move(bs);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          BigStorage bs{BigTrack(count)};
          SmallStorage ss{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          ss = std::move(bs);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
      }
    }
  }
  SECTION("move small into big storage") {
    SECTION("move ctor") {
      SECTION("move construct from empty storage") {
        SmallStorage ss{};
        BigStorage bs{std::move(ss)};
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
      }
      SECTION("move construct from non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          SmallStorage ss{Track(count)};
          CHECK(count == 1);
          BigStorage bs{std::move(ss)};
          CHECK(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          SmallStorage ss{MediumTrack(count)};
          CHECK(count == 1);
          BigStorage bs{std::move(ss)};
          CHECK(count == 1);
        }
        SECTION("object stays on heap") {
          int count = 0;
          SmallStorage ss{BigTrack(count)};
          CHECK(count == 1);
          BigStorage bs{std::move(ss)};
          CHECK(count == 1);
        }
      }
    }
    SECTION("move assignment") {
      SECTION("move assign empty into empty storage") {
        SmallStorage ss{};
        BigStorage bs{};
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
        bs = std::move(ss);
        CHECK(ss.data() == nullptr);
        CHECK(bs.data() == nullptr);
      }
      SECTION("move assign non empty into empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          SmallStorage ss{Track(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = std::move(ss);
          CHECK(count == 1);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          SmallStorage ss{MediumTrack(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = std::move(ss);
          CHECK(count == 1);
        }
        SECTION("object stays on heap") {
          int count = 0;
          SmallStorage ss{BigTrack(count)};
          CHECK(count == 1);
          BigStorage bs{};
          bs = std::move(ss);
          CHECK(count == 1);
        }
      }
      SECTION("move assign non empty into non empty storage") {
        SECTION("object fits both buffers") {
          int count = 0;
          int count2 = 0;
          SmallStorage ss{Track(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = std::move(ss);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
        SECTION("object fits bigger buffer") {
          int count = 0;
          int count2 = 0;
          SmallStorage ss{MediumTrack(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = std::move(ss);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
        SECTION("object stays on heap") {
          int count = 0;
          int count2 = 0;
          SmallStorage ss{BigTrack(count)};
          BigStorage bs{Track(count2)};
          CHECK(count == 1);
          CHECK(count2 == 1);
          bs = std::move(ss);
          CHECK(count == 1);
          CHECK(count2 == 0);
        }
      }
    }
  }
}
