// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty, typename... Args>
concept one_of = (std::same_as<Ty, Args> || ...);
}
