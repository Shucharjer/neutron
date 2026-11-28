#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/ecs/concepts/stage.hpp"
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <stage Stage, auto Sys, typename T>
struct construct_from_world_t {
    template <internal::world World>
    constexpr T operator()(World& world) const
        noexcept(std::is_nothrow_constructible_v<T, World>) {
        return T(world);
    }
};

ATOM_EXPORT template <stage Stage, auto Sys, typename T>
inline constexpr construct_from_world_t<Stage, Sys, T> construct_from_world;

} // namespace neutron
