#pragma once

// IWYU pragma: begin_exports

#include "neutron/detail/macros.hpp"

#if ATOM_HAS_REFLECTION && false
    #include "neutron/detail/reflection/meta/hash_of.hpp"
    #include "neutron/detail/reflection/meta/name_of.hpp"
    #include "neutron/detail/reflection/meta/reserve.hpp"

#else
    #include "neutron/detail/reflection/legacy/concepts.hpp"
    #include "neutron/detail/reflection/legacy/destroy.hpp"
    #include "neutron/detail/reflection/legacy/member_count.hpp"
    #include "neutron/detail/reflection/legacy/member_names.hpp"
    #include "neutron/detail/reflection/legacy/member_offset.hpp"
    #include "neutron/detail/reflection/legacy/member_traits.hpp"
    #include "neutron/detail/reflection/legacy/member_type.hpp"
    #include "neutron/detail/reflection/legacy/reflected.hpp"
    #include "neutron/detail/reflection/legacy/tuple_view.hpp"
    #include "neutron/detail/reflection/legacy/user_macros.hpp"

namespace neutron {

using _refl_legacy::name_of;
using _refl_legacy::hash_of;
using _refl_legacy::member_count_of;
using _refl_legacy::member_names_of;
using _refl_legacy::offsets_of;
using _refl_legacy::offset_value_of;
using _refl_legacy::member_type_of;
using _refl_legacy::member_type_of_t;
using _refl_legacy::basic_reflected;
using _refl_legacy::reflected;

} // namespace neutron

#endif

// IWYU pragma: end_exports
