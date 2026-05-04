// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <typename Ty>
concept nothrow_conditional_move_constrctible =
    std::is_nothrow_move_constructible_v<Ty> ||
    std::is_nothrow_copy_constructible_v<Ty>;

}
