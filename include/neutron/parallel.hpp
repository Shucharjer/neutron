#pragma once
// IWYU pragma: begin_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else
    #include "neutron/detail/parallel/cpu_relax.hpp"
    #include "neutron/detail/parallel/spin_mutex.hpp"
    #include "neutron/detail/parallel/spinlock.hpp"
#endif

// IWYU pragma: end_exports
