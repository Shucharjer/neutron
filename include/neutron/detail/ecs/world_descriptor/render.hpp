// IWYU pragma: private, include "neutron/detail/ecs/world_descriptor.hpp"
#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

namespace _n_render {

struct _enable_render_t : descriptor_adaptor_closure<_enable_render_t> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return insert_tagged_type_list_inplace_t<
            _enable_render_t, Descriptor, _enable_render_t>{};
    }
};

} // namespace _n_render

inline constexpr _n_render::_enable_render_t enable_render;

} // namespace neutron
