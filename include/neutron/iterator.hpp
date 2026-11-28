#pragma once
// IWYU pragma: begin_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else
    #include "neutron/detail/iterator/concepts.hpp"
    #include "neutron/detail/iterator/iter_wrapper.hpp"
    #include "neutron/detail/iterator/phony_input_iterator.hpp"
#endif

// IWYU pragma: end_exports
