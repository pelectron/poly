#ifndef POLY_ALLOC_HPP
#define POLY_ALLOC_HPP
#include "poly/config.hpp"

#include <memory>
#include <new>
#include <type_traits>

#if defined(POLY_ON_WINDOWS) && defined(POLY_HEADER_ONLY)
#  include <malloc.h>
#elif defined(POLY_HEADER_ONLY)
#  include <cstdlib>
#endif

namespace poly::detail {

#if __STDC_HOSTED__ == 1
// in hosted environments, mem_alloc and mem_free are implemented -> heap_storage is available.
#  ifdef POLY_HEADER_ONLY
#    ifdef POLY_ON_WINDOWS
/// allocates uninitialized memory with size `size` and alignment `align` using
/// operator new(size_t,std::align_val_t, std::nothrow_t).
/// @returns pointer to allocated memory on success. Returns nullptr if
/// allocation failed.
[[nodiscard]] inline void* mem_alloc(std::size_t size,
                                     std::size_t align) noexcept {
  return _aligned_malloc(size, align);
}

/// deallocates memory allocated with me_alloc().
/// @param p pointer returned by mem_alloc(size,align)
/// @param align must be the same alignment value passed in to mem_alloc().
inline void mem_free(void* p) noexcept { _aligned_free(p); }
#    else
/// allocates uninitialized memory with size `size` and alignment `align` using
/// operator new(size_t,std::align_val_t, std::nothrow_t).
/// @returns pointer to allocated memory on success. Returns nullptr if
/// allocation failed.
[[nodiscard]] inline void* mem_alloc(std::size_t size,
                                     std::size_t align) noexcept {
  using namespace std; // std::aligned_alloc is not available on some platforms
                       // even in C++17, but ::aligned_alloc should be present.
  return aligned_alloc(align, size);
}

/// deallocates memory allocated with mem_alloc().
/// @param p pointer returned by mem_alloc(size,align)
inline void mem_free(void* p) noexcept { std::free(p); }
#    endif
#  else
/// allocates uninitialized memory with size `size` and alignment `align`
[[nodiscard]] POLY_API void*
mem_alloc(std::size_t size,
          std::size_t align = __STDCPP_DEFAULT_NEW_ALIGNMENT__) noexcept;

/// deallocates memory allocated with mem_alloc().
/// @param p pointer returned by mem_alloc(size,align)
POLY_API void mem_free(void* p) noexcept;

#  endif
#else

// in freestanding mode, dynamic allocation is not supported by default and
// must be enabled by implementing poly::detail::mem_alloc and
// poly::detail::mem_free.

[[nodiscard]] POLY_API void* mem_alloc(std::size_t size, std::size_t align);
POLY_API void mem_free(void* p) noexcept;
#endif

#if __cplusplus >= 202002L
template<class T, class... Args>
constexpr T* construct_at(T* p, Args&&... args) {
  return std::construct_at(p, std::forward<Args>(args)...);
}
#else
template<class T, class... Args>
T* construct_at(T* p, Args&&... args) {
  return ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
}
#endif

/// allocates a T on the heap and constructs it with args.
///
/// @note memory allocated with allocate() must be freed with deallocate()
///
/// @param args parameters to construct a T with.
/// @tparam T type to allocate
/// @tparam Args argument types
/// @returns pointer to T if allocation succeeded, nullptr if allocation failed.
///
template<typename T, typename... Args>
[[nodiscard]] T* allocate(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args&&...>) {
  void* mem = mem_alloc(sizeof(T), alignof(T));
  if (!mem)
    return nullptr;
  return construct_at(static_cast<T*>(mem), std::forward<Args>(args)...);
}

/// destroys the object located at T and frees its memory.
///
/// @param p pointer to T allocated with allocate<T>(...)
/// @param T type of object to deallocate
template<typename T>
void deallocate(T* p) noexcept(std::is_nothrow_destructible_v<T>) {
  std::destroy_at(p);
  mem_free(p);
}
} // namespace poly::detail
#endif
