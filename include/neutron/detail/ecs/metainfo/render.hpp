// IWYU pragma: private, include "neutron/detail/ecs/metainfo.hpp"
#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/world_descriptor/render.hpp"

namespace neutron {

namespace _metainfo {

template <typename RenderInfo>
struct _validate_renderinfo {
    using markers                        = typename RenderInfo::type_list;
    static constexpr std::size_t enable_count = type_list_size_v<markers>;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct fetch_renderinfo {
    using type       = fetch_tinfo_t<_n_render::_enable_render_t, Descriptor>;
    using validation = _validate_renderinfo<type>;

    static_assert(
        validation::value,
        "invalid render policies: enable_render may only be specified once");
};

template <typename Descriptor>
using fetch_renderinfo_t = typename fetch_renderinfo<Descriptor>::type;

template <typename>
struct renderinfo_traits;

template <typename... Markers>
struct renderinfo_traits<
    tagged_type_list<_n_render::_enable_render_t, Markers...>> {
    using tagged_type =
        tagged_type_list<_n_render::_enable_render_t, Markers...>;
    using type_list = typename tagged_type::type_list;

    static constexpr std::size_t enable_count = type_list_size_v<type_list>;
    static constexpr bool is_enabled     = enable_count != 0;
    static constexpr bool value          = enable_count <= 1;
};

template <typename Descriptor>
struct render_info : renderinfo_traits<fetch_renderinfo_t<Descriptor>> {
    static constexpr bool value =
        _validate_renderinfo<fetch_renderinfo_t<Descriptor>>::value;
};

} // namespace _metainfo

using _metainfo::fetch_renderinfo;
using _metainfo::fetch_renderinfo_t;
using _metainfo::render_info;
using _metainfo::renderinfo_traits;

} // namespace neutron
