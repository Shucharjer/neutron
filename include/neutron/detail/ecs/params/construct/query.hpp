#pragma once
#include "neutron/detail/ecs/compile-time/collect_params/query.hpp"
#include "neutron/detail/ecs/concepts/stage.hpp"
#include "neutron/detail/ecs/core/world_accessor.hpp"
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/ecs/params/construct.hpp"
#include "neutron/detail/metafn/has.hpp"

namespace neutron {

template <stage Stage, auto Sys, typename... Filters>
struct construct_from_world_t<Stage, Sys, query<Filters...>> {
    using arg_t = query<Filters...>;

    template <internal::world World>
    auto operator()(World& world) {
        using alloc_t = typename World::allocator_type;
        using params  = query_cache_of<typename World::descriptor_type>;
        if constexpr (type_list_has_v<params, arg_t>) {
            return get_first<arg_t>(world_accessor::queries(world));
        }
    }
};

} // namespace neutron
