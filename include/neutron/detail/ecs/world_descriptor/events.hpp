// IWYU pragma: private, include "neutron/detail/ecs/world_descriptor.hpp"
#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

namespace _n_events {

struct _enable_events_t : descriptor_adaptor_closure<_enable_events_t> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return insert_tagged_type_list_inplace_t<
            _enable_events_t, Descriptor, _enable_events_t>{};
    }
};

} // namespace _n_events

inline constexpr _n_events::_enable_events_t enable_events;

} // namespace neutron
