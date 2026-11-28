#pragma once

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else

// IWYU pragma: begin_exports

    #include "neutron/detail/reflection/refl.hpp"

    #include "neutron/detail/reflection/hash.hpp"
    #include "neutron/detail/reflection/type_traits.hpp"

    #include "neutron/detail/reflection/serialize.hpp"

    #include "neutron/detail/reflection/legacy/third_party/serialization/json/nlohmann.hpp"
    #include "neutron/detail/reflection/legacy/third_party/serialization/json/simdjson.hpp"

    #include "neutron/detail/reflection/legacy/third_party/bind/lua/sol2.hpp"

// IWYU pragma: end_exports

#endif
