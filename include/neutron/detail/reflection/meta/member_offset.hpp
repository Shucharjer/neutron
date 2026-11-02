#pragma once
#include <vector>
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <array>
    #include <meta>
    #include <tuple>
    #include "neutron/detail/reflection/offset_helper.hpp"

namespace neutron {

template <std::meta::info Info>
consteval auto _get_offsets_type() -> std::meta::info {
    using namespace std::meta;
    constexpr auto ctx  = access_context::unchecked();
    constexpr auto mbrs = nonstatic_data_members_of(Info, ctx);
    using class_type    = [:type_of(Info):];
    std::vector<info> mptrs;
    template for (constexpr auto mbr : mbrs) {
        using mbr_t = [:type_of(mbr):];
        using ptr_t = mbr_t class_type::*;
        mptrs.emplace_back(^^ptr_t);
    }
    return substitute(^^std::tuple, mptrs);
}

template <std::meta::info Info, std::meta::info Class>
consteval auto _get_offset() {
    using namespace std::meta;
    std::ptrdiff_t offset = offset_of(Info).bytes;
    auto mptr             = _refl::_offset_mapping(offset);
    using mbr_t           = typename[:type_of(Info):];
    using class_type      = typename[:type_of(Class):];
    mbr_t class_type::* t = mptr;
}

template <std::meta::info Info>
consteval auto _offsets_of() {
    using namespace std::meta;
    constexpr auto ctx  = access_context::unchecked();
    constexpr auto mbrs = nonstatic_data_members_of(Info, ctx);
    using class_type    = [:type_of(Info):];
    [:type_of(_get_offsets_type<Info>()):] r;
    template for (constexpr auto mbr : mbrs) { _get_offset<mbr, Info>();
        
     }
}

template <typename T>
constexpr auto offsets_of() {
    return [] consteval {
        constexpr std::size_t count = member_count_of<T>();
        using namespace std::meta;
        constexpr auto ctx = access_context::unchecked();
        std::array<std::size_t, count> r;
        auto mbrs = nonstatic_data_members_of(^^T, ctx);
        for (std::size_t i = 0; i < count; ++i) {
            r[i] = offset_of(mbrs[i]).bytes;
        }
        return r;
    }();
}

} // namespace neutron

#endif
