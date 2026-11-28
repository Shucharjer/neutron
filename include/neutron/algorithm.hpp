#pragma once
// IWYU pragma: begin_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else
    #include "neutron/detail/algorithm/branchless_lower_bound.hpp"
    #include "neutron/detail/algorithm/inplace_merge.hpp"
    #include "neutron/detail/algorithm/minmax.hpp"
#endif

// IWYU pragma: end_exports
