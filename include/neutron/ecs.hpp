#pragma once
// IWYU pragma: begin_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else
    #include "neutron/detail/ecs/compile-time.hpp"
    #include "neutron/detail/ecs/concepts.hpp"
    #include "neutron/detail/ecs/core.hpp"
    #include "neutron/detail/ecs/executor.hpp"
    #include "neutron/detail/ecs/fwd.hpp"
    #include "neutron/detail/ecs/params.hpp"
    #include "neutron/detail/ecs/runtime.hpp"
    #include "neutron/detail/ecs/snapshot.hpp"
    #include "neutron/detail/ecs/utility.hpp"
#endif

// IWYU pragma: end_exports
