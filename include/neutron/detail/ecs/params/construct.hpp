#pragma once
#include <type_traits>
#include "neutron/detail/ecs/concepts/stage.hpp"
#include "neutron/detail/ecs/fwd.hpp"

namespace neutron {

template <stage Stage, auto Sys, typename T>
struct construct_from_world_t {
    template <internal::world World>
    constexpr T operator()(World& world) const
        noexcept(std::is_nothrow_constructible_v<T, World>) {
        return T(world);
    }
};

template <stage Stage, auto Sys, typename T>
inline constexpr construct_from_world_t<Stage, Sys, T> construct_from_world;

} // namespace neutron
