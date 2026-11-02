#pragma once
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <cstddef>
    #include <meta>
namespace neutron {

template <std::meta::info Info, std::size_t I>
consteval auto _member_type_of_impl() {
    using namespace std::meta;
    constexpr auto ctx     = access_context::unchecked();
    constexpr auto mbrinfo = nonstatic_data_members_of(Info, ctx)[I];
    return static_cast<[:type_of(mbrinfo):]*>(nullptr);
}

template <std::size_t I, typename T>
using member_type_of_t =
    std::remove_pointer_t<decltype(_member_type_of_impl<^^T, I>())>;

} // namespace neutron

#endif
