#pragma once

// IWYU pragma: begin_exports

#include "neutron/detail/macros.hpp"

#if ATOM_HAS_REFLECTION && false
    #include "neutron/detail/reflection/meta/name_of.hpp"
    #include "neutron/detail/reflection/meta/hash_of.hpp"
    #include "neutron/detail/reflection/meta/reserve.hpp"
#else
    #include "neutron/detail/reflection/legacy/access.hpp"
    #include "neutron/detail/reflection/legacy/concepts.hpp"
    #include "neutron/detail/reflection/legacy/destroy.hpp"
    #include "neutron/detail/reflection/legacy/member_count.hpp"
    #include "neutron/detail/reflection/legacy/member_names.hpp"
    #include "neutron/detail/reflection/legacy/member_offset.hpp"
    #include "neutron/detail/reflection/legacy/member_traits.hpp"
    #include "neutron/detail/reflection/legacy/member_type.hpp"
    #include "neutron/detail/reflection/legacy/reflected.hpp"
    #include "neutron/detail/reflection/legacy/tuple_view.hpp"
    #include "neutron/detail/reflection/legacy/type_traits.hpp"
    #include "neutron/detail/reflection/legacy/user_macros.hpp"
#endif

// IWYU pragma: end_exports
