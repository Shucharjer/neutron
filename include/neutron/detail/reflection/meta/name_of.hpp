// private, include <neutron/reflection.hpp>
#pragma once
#include "neutron/detail/macros.hpp"

#if ATOM_HAS_REFLECTION

#include <meta>
#include <string_view>

namespace neutron {

template <typename Ty>
consteval std::string_view name_of() noexcept {
    constexpr auto info = ^^Ty;
    if constexpr (std::meta::is_type_alias(info)) {
        constexpr auto underlying = std::meta::underlying_type(info);
        using type                = [:std::meta::type_of(underlying):];
        return name_of<type>();
    }
    return std::meta::display_string_of(^^Ty);
}

} // namespace neutron

#endif