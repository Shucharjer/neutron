// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <array>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty, typename... Args>
consteval auto make_array(Args&&... args) {
    return std::array<Ty, sizeof...(Args)>{ Ty{ std::forward<Args>(args) }... };
}

} // namespace neutron

ATOM_EXPORT using neutron::make_array;
