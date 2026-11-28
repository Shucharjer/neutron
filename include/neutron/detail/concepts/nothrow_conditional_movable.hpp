// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty>
concept nothrow_conditional_move_constrctible =
    std::is_nothrow_move_constructible_v<Ty> ||
    std::is_nothrow_copy_constructible_v<Ty>;
}
