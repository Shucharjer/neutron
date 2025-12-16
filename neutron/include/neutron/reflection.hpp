#pragma once

#if __has_feature(reflection) && __cplusplus >= 202602L && false

    #include "../src/neutron/internal/reflection/meta/reserve.hpp" // IWYU pragma: export

#else

    #include "../src/neutron/internal/reflection/legacy/access.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/concepts.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/destroy.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/member_count.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/member_names.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/member_offset.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/member_traits.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/member_type.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/reflected.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/tuple_view.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/type_traits.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/user_macros.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/reflection/legacy/third_party/serialization/json/nlohmann.hpp" // IWYU pragma: export
    #include "../src/neutron/internal/reflection/legacy/third_party/serialization/json/simdjson.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/reflection/legacy/third_party/bind/lua/sol2.hpp" // IWYU pragma: export

#endif
