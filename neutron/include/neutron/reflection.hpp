#pragma once

// IWYU pragma: begin_exports

#if __has_feature(reflection) && __cplusplus >= 202602L && false

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

    #include "neutron/detail/reflection/legacy/third_party/serialization/json/nlohmann.hpp"
    #include "neutron/detail/reflection/legacy/third_party/serialization/json/simdjson.hpp"

    #include "neutron/detail/reflection/legacy/third_party/bind/lua/sol2.hpp"

#endif

// IWYU pragma: end_exports
