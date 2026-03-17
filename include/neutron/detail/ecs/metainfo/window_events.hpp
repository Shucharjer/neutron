#pragma once
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/world_descriptor/window_events.hpp"

namespace neutron {

namespace _metainfo {

template <typename RenderInfo>
struct _validate_wndev {
    using markers                        = typename RenderInfo::type_list;
    static constexpr size_t enable_count = type_list_size_v<markers>;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct fetch_wndev {
    using type = fetch_tinfo_t<
        _enable_window_events::_enable_window_events_t, Descriptor>;
    using validation = _validate_renderinfo<type>;

    static_assert(
        validation::value,
        "invalid render policies: enable_render may only be specified once");
};

template <typename Descriptor>
using fetch_wndev_t = typename fetch_wndev<Descriptor>::type;

template <typename>
struct wndev_traits;

template <typename... Markers>
struct wndev_traits<tagged_type_list<
    _enable_window_events::_enable_window_events_t, Markers...>> {
    using tagged_type = tagged_type_list<
        _enable_window_events::_enable_window_events_t, Markers...>;
    using type_list = typename tagged_type::type_list;

    static constexpr size_t enable_count = type_list_size_v<type_list>;
    static constexpr bool is_enabled     = enable_count != 0;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct wndev_info : wndev_traits<fetch_wndev_t<Descriptor>> {
    static constexpr bool value =
        _validate_wndev<fetch_wndev_t<Descriptor>>::value;
};

} // namespace _metainfo

using _metainfo::fetch_wndev;
using _metainfo::fetch_wndev_t;
using _metainfo::wndev_info;
using _metainfo::wndev_traits;

} // namespace neutron
