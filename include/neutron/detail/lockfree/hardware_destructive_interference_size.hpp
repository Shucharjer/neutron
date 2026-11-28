#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <new>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

#ifdef __clang__
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Winterference-size"
#endif

ATOM_EXPORT inline constexpr std::size_t hdi_size =
    std::hardware_destructive_interference_size;

#ifdef __clang__
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

} // namespace neutron
