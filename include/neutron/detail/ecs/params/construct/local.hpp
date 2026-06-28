#pragma once
#include "neutron/detail/ecs/compile-time/collect_params/local.hpp"
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/ecs/params/construct.hpp"
#include "neutron/detail/ecs/params/local.hpp"

namespace neutron {
template <stage Stage, auto Sys, typename... Args>
struct construct_from_world_t<Stage, Sys, local<Args...>> {
    template <internal::world World>
    auto operator()(World& world) const noexcept -> local<Args...> {
        using locals  = local_of<typename World::descriptor_type>;
        using tuple_t = systuple<Stage, Sys, Args...>;
        return get_first<tuple_t>(world_accessor::locals(world));
    }
};
} // namespace neutron
