#pragma once
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <cstddef>
    #include <meta>

namespace neutron {

template <typename T>
constexpr std::size_t member_count_of() {
    using namespace std::meta;
    constexpr auto ctx = access_context::unchecked();
    return nonstatic_data_members_of(^^T, ctx).size();
}

} // namespace neutron

#endif
