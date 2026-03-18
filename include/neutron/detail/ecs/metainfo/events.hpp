#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/world_descriptor/events.hpp"

namespace neutron {

namespace _metainfo {

template <typename EventsInfo>
struct _validate_eventsinfo {
    using markers                        = typename EventsInfo::type_list;
    static constexpr size_t enable_count = type_list_size_v<markers>;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct fetch_eventsinfo {
    using type       = fetch_tinfo_t<_n_events::_enable_events_t, Descriptor>;
    using validation = _validate_eventsinfo<type>;

    static_assert(
        validation::value,
        "invalid events policies: enable_events may only be specified once");
};

template <typename Descriptor>
using fetch_eventsinfo_t = typename fetch_eventsinfo<Descriptor>::type;

template <typename>
struct eventsinfo_traits;

template <typename... Markers>
struct eventsinfo_traits<
    tagged_type_list<_n_events::_enable_events_t, Markers...>> {
    using tagged_type =
        tagged_type_list<_n_events::_enable_events_t, Markers...>;
    using type_list = typename tagged_type::type_list;

    static constexpr size_t enable_count = type_list_size_v<type_list>;
    static constexpr bool is_enabled     = enable_count != 0;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct events_info : eventsinfo_traits<fetch_eventsinfo_t<Descriptor>> {
    static constexpr bool value =
        _validate_eventsinfo<fetch_eventsinfo_t<Descriptor>>::value;
};

} // namespace _metainfo

using _metainfo::events_info;
using _metainfo::eventsinfo_traits;
using _metainfo::fetch_eventsinfo;
using _metainfo::fetch_eventsinfo_t;

} // namespace neutron
