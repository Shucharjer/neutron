// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <cstdint>

namespace neutron {

#ifndef UTILS_LONG_ID
using id_t = uint32_t;
#else
using id_t = uint64_t;
#endif

} // namespace neutron

using neutron::id_t;
