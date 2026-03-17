// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

namespace _enable_window_events {

struct _enable_window_events_t {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return insert_tagged_type_list_inplace_t<
            _enable_window_events_t, Descriptor, _enable_window_events_t>{};
    }
};

} // namespace _enable_window_events

inline constexpr _enable_window_events::_enable_window_events_t
    enable_window_events;

} // namespace neutron
