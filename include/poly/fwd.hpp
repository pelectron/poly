#ifndef POLY_FWD_HPP
#define POLY_FWD_HPP
#include "poly/config.hpp"

#include "poly/traits.hpp"

namespace poly {

template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
class basic_object;

template <POLY_STORAGE StorageType, POLY_TYPE_LIST PropertySpecs,
          POLY_TYPE_LIST MethodSpecs>
class basic_interface;

class ref_storage;

template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class local_storage;

template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class move_only_local_storage;

template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class sbo_storage;

template <std::size_t Size, std::size_t Alignment = alignof(std::max_align_t)>
class move_only_sbo_storage;

template <typename... Ts> class variant_storage;
} // namespace poly
#endif
