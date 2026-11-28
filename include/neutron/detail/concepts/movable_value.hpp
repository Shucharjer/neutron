#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty>
concept movable_value = std::move_constructible<Ty> &&
                        std::constructible_from<std::decay_t<Ty>, Ty>;
}
