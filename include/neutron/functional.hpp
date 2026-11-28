#pragma once
// IWYU pragma: begin_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else
    #include "neutron/detail/functional/delegate.hpp"
    #include "neutron/detail/functional/dispatcher.hpp"
    #include "neutron/detail/functional/sink.hpp"
#endif

// IWYU pragma: end_exports
