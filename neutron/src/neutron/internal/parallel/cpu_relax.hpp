#pragma once

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)
    #include <xmmintrin.h>
#endif

/*! @cond TURN_OFF_DOXYGEN */
namespace neutron::internal {

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)
inline void cpu_relax() { _mm_pause(); }
#elif defined(__aarch64__)
inline void cpu_relax() { __asm__ volatile("yield"); }
#else
inline void cpu_relax() {}
#endif

const auto max_spin_time = 1024;

} // namespace neutron::internal
/*! @endcond */
