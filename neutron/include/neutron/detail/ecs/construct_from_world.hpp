// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

template <auto Sys, typename Argument, size_t IndexOfSysInSysList = 0>
struct construct_from_world_t {
    template <world World>
    constexpr Argument operator()(World& world) const {
        return Argument{ world };
    }
};

template <auto Sys, typename Arg, size_t IndexOfSysInSysList = 0>
constexpr inline construct_from_world_t<Sys, Arg, IndexOfSysInSysList>
    construct_from_world;

template <auto Sys, typename Arg, size_t Index = 0>
concept constructible_from_world = requires {
    {
        construct_from_world<Sys, Arg, Index>(
            std::declval<basic_world<world_descriptor_t<>>>())
    } -> std::same_as<Arg>;
};

} // namespace neutron
