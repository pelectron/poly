#include "poly.hpp"
#include <cstdlib>

#ifndef POLY_HEADER_ONLY
namespace poly {

namespace detail {

#  ifdef POLY_ON_WINDOWS
  [[nodiscard]] POLY_API void* mem_alloc(std::size_t size, std::size_t align) noexcept {
    return _aligned_malloc(size, align);
  }

  POLY_API void mem_free(void* p) noexcept { _aligned_free(p); }
#  else
  [[nodiscard]] POLY_API void* mem_alloc(std::size_t size, std::size_t align) noexcept {
    return std::aligned_alloc(size, align);
  }

  POLY_API void mem_free(void* p) noexcept { std::free(p); }
#  endif

} // namespace detail
} // namespace poly
#endif
