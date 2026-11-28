// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstdint>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT using hash_t      = std::uint32_t;
ATOM_EXPORT using long_hash_t = std::uint64_t;

} // namespace neutron
