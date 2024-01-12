#include "poly.hpp"
#include <catch2/catch_all.hpp>
POLY_METHOD(method);
POLY_PROPERTY(property);
using OBJ = poly::Object<PROPERTIES(property(int)), METHODS(int(method)), 32>;

struct S1 {
  int property;
};

int extend(method, S1 &s) { return 0; }

struct S2 {};
int extend(method, S2 &s) { return 1; }

int get(property, const S2 &s) { return 5; }

void set(property, S2 &, const int &) {}

TEST_CASE("Object") {}
