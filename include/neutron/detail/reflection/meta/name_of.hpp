// private, include <neutron/reflection.hpp>
#pragma once
#include "neutron/detail/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <meta>
    #include <string_view>

namespace neutron {

template <typename Ty>
consteval std::string_view name_of() noexcept {
    using namespace std::meta;
    auto info = ^^Ty;
    while (is_type_alias(info)) {
        info = underlying_type(info);
    }
    if (has_identifier(info)) {
        return identifier_of(info);
    }
    return display_string_of(info);
}

} // namespace neutron

#endif
