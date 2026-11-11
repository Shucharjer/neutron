// IWYU pragma: private, include <neutron/ecs.hpp>
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
    auto operator()(World& world) const -> arg_t {
        using params = query_cache_of<typename World::descriptor_type>;
        static_assert(
            type_list_has_v<params, arg_t>,
            "the query is not requested by any system of this world");

        using querior_t =
            typename arg_t::template querior_type<typename World::allocator_type>;
        auto& querior = get_first<querior_t>(world_accessor::queries(world));
        querior.sync(world);
        return arg_t{ querior };
    }
};

} // namespace neutron
