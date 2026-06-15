#pragma once
#include <cstddef>
#include <new>

namespace neutron {

#ifdef __clang__
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Winterference-size"
#endif

constexpr std::size_t hdi_size = std::hardware_destructive_interference_size;

#ifdef __clang__
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

} // namespace neutron

