#pragma once
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #ifndef ATOM_BUILDING_MODULE
        #include <cstddef>
        #include <meta>
    #endif

namespace neutron {

ATOM_EXPORT template <typename T>
constexpr std::size_t member_count_of() {
    using namespace std::meta;
    constexpr auto ctx = access_context::unchecked();
    return nonstatic_data_members_of(^^T, ctx).size();
}

} // namespace neutron

#endif
