#include <neutron/ecs.hpp>

using namespace neutron;

int main() {

    using valid_events_raw =
        fetch_eventsinfo_t<decltype(world_desc | enable_events)>;
    using invalid_events_raw = fetch_tinfo_t<
        _n_events::_enable_events_t,
        decltype(world_desc | enable_events | enable_events)>;
    static_assert(eventsinfo_traits<valid_events_raw>::enable_count == 1);
    static_assert(!_metainfo::_validate_eventsinfo<invalid_events_raw>::value);

    using valid_render_raw =
        fetch_renderinfo_t<decltype(world_desc | enable_render)>;
    using invalid_render_raw = fetch_tinfo_t<
        _n_render::_enable_render_t,
        decltype(world_desc | enable_render | enable_render)>;
    static_assert(!_metainfo::_validate_renderinfo<invalid_render_raw>::value);
    return 0;
}
