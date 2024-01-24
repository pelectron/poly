#include "poly/function.hpp"
#include "catch2/catch_all.hpp"
#include "poly/storage.hpp"
using Fn = poly::function<int(int) const,poly::local_storage<32, 8>>;
// using Fn2 = poly::any_function<poly::local_storage<32, 8>, int(int) const,
//                                int(float), float(double)>;

template <typename... Ts> struct overload : public Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts...) -> overload<std::decay_t<Ts>...>;

TEST_CASE("function") {
  Fn f{[](int i) -> int { return i - 1; }};
  REQUIRE(f(43) == 42);
  // Fn2 f2{overload{[](int i) -> int { return i - 1; },
  //                 [](float f) mutable -> int { return f + 1; },
  //                 [](double d) mutable -> float { return d + 2; }}};
  // REQUIRE(f2(43i32) == 42);
  // REQUIRE(f2(43.0f) == 44);
  // REQUIRE(f2(43.0) == 45.0f);
  // f.bind([](int i) -> int { return i + 1; });
  // REQUIRE(f(43) == 44);
}
