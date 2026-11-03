#pragma once
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <array>
    #include <ranges>
    #include <string_view>
    #include "neutron/detail/reflection/meta/member_count.hpp"

namespace neutron {

template <typename T>
constexpr std::array<std::string_view, member_count_of<T>()> member_names_of() {
    return [] consteval {
        constexpr std::size_t count = member_count_of<T>();
        using namespace std::meta;
        constexpr auto ctx = access_context::unchecked();
        std::array<std::string_view, count> r;
        auto mbrs = nonstatic_data_members_of(^^T, ctx);
        for (std::size_t i = 0; i < count; ++i) {
            // display_string_of is implementation-defined (GCC qualifies it)
            r[i] = has_identifier(mbrs[i]) ? identifier_of(mbrs[i])
                                           : std::string_view{};
        }
        return r;
    }();
}

} // namespace neutron

#endif
