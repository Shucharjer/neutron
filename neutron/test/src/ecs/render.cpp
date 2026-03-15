#include <neutron/ecs.hpp>

using namespace neutron;

int main() {

    using valid_render_raw =
        fetch_renderinfo_t<decltype(world_desc | enable_render)>;
    using invalid_render_raw = fetch_tinfo_t<
        _n_render::_enable_render_t,
        decltype(world_desc | enable_render | enable_render)>;
    static_assert(!_metainfo::_validate_renderinfo<invalid_render_raw>::value);
    return 0;
}
