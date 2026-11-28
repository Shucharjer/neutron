#pragma once

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)
    #ifndef ATOM_BUILDING_MODULE
        #include <xmmintrin.h>
    #endif
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)

ATOM_EXPORT inline void cpu_relax() { _mm_pause(); }

#elif defined(__aarch64__)

inline void cpu_relax() { __asm__ volatile("yield"); }

#else

inline void cpu_relax() {}

#endif

ATOM_EXPORT inline constexpr auto max_spin_time = 1024;

} // namespace neutron
