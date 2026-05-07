#pragma once
#include "neutron/detail/ecs/metainfo/render.hpp"

namespace neutron {

inline constexpr struct get_render_t {
    template <typename Desc>
    consteval bool operator()(Desc) const noexcept {
        return _metainfo::render_info<Desc>::is_enabled;
    }
} get_render;

} // namespace neutron
