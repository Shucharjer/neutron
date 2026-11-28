// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstdint>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

#ifndef UTILS_LONG_ID
ATOM_EXPORT using id_t = std::uint32_t;
#else
ATOM_EXPORT using id_t = std::uint64_t;
#endif

} // namespace neutron

ATOM_EXPORT using neutron::id_t;
