#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <string_view>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename StringView>
concept string_view_like = std::convertible_to<StringView, std::string_view>;
}
