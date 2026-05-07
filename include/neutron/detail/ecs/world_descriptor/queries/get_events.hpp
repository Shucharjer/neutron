#pragma once
#include "neutron/detail/ecs/metainfo/events.hpp"

namespace neutron {

inline constexpr struct get_events_t {
    template <typename Desc>
    consteval bool operator()(Desc) const noexcept {
        return _metainfo::events_info<Desc>::is_enabled;
    }
} get_events;

} // namespace neutron
