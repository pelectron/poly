#ifndef POLY_ALWAYS_FALSE_HPP
#define POLY_ALWAYS_FALSE_HPP

namespace poly::detail{
  template<typename >
  inline constexpr bool always_false = false;
}
#endif
