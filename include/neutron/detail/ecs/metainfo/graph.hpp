// IWYU pragma: private, include "neutron/detail/ecs/metainfo.hpp"
#pragma once
#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo/events.hpp"
#include "neutron/detail/ecs/metainfo/render.hpp"
#include "neutron/detail/ecs/metainfo/sysinfo.hpp"
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/tuple/first.hpp"

namespace neutron {

namespace _metainfo {

using enum stage;

template <typename Descriptor>
struct _descriptor_sys_traits {
    using prestartups =
        typename stage_sysinfo<pre_startup, Descriptor>::traits_list;
    using startups = typename stage_sysinfo<startup, Descriptor>::traits_list;
    using poststartups =
        typename stage_sysinfo<post_startup, Descriptor>::traits_list;

    using firsts  = typename stage_sysinfo<first, Descriptor>::traits_list;
    using eventss = typename stage_sysinfo<events, Descriptor>::traits_list;

    using preupdates =
        typename stage_sysinfo<pre_update, Descriptor>::traits_list;
    using updates = typename stage_sysinfo<update, Descriptor>::traits_list;
    using postupdates =
        typename stage_sysinfo<post_update, Descriptor>::traits_list;

    using renders = typename stage_sysinfo<render, Descriptor>::traits_list;

    using lasts     = typename stage_sysinfo<last, Descriptor>::traits_list;
    using shutdowns = typename stage_sysinfo<shutdown, Descriptor>::traits_list;

    using all = type_list_cat_t<
        prestartups, startups, poststartups, firsts, eventss, preupdates,
        updates, postupdates, renders, lasts, shutdowns>;
};

template <typename Node, typename Remaining>
struct _has_unresolved_predecessor;

template <
    typename Node, template <typename...> typename Template, typename... Others>
struct _has_unresolved_predecessor<Node, Template<Others...>> :
    std::bool_constant<(
        false || ... ||
        (!std::same_as<Node, Others> && _direct_before_v<Others, Node>))> {};

template <typename Node, typename Remaining>
constexpr bool _source_node_v =
    !_has_unresolved_predecessor<Node, Remaining>::value;

template <typename Current, typename Iter, typename Remaining>
struct _collect_parallel_sources_impl;

template <typename... Selected, typename Remaining>
struct _collect_parallel_sources_impl<
    type_list<Selected...>, type_list<>, Remaining> {
    using type = type_list<Selected...>;
};

template <
    typename... Selected, typename Head, typename... Tail, typename Remaining>
struct _collect_parallel_sources_impl<
    type_list<Selected...>, type_list<Head, Tail...>, Remaining> {
    static constexpr bool selectable =
        _source_node_v<Head, Remaining> && !Head::execute_traits::is_individual;

    using next = std::conditional_t<
        selectable, type_list<Selected..., Head>, type_list<Selected...>>;
    using type = typename _collect_parallel_sources_impl<
        next, type_list<Tail...>, Remaining>::type;
};

template <typename Remaining>
using _parallel_source_set_t = typename _collect_parallel_sources_impl<
    type_list<>, Remaining, Remaining>::type;

template <typename Iter, typename Remaining>
struct _first_source_impl;

template <typename Remaining>
struct _first_source_impl<type_list<>, Remaining> {
    using type = type_list<>;
};

template <typename Head, typename... Tail, typename Remaining>
struct _first_source_impl<type_list<Head, Tail...>, Remaining> {
    using type = std::conditional_t<
        _source_node_v<Head, Remaining>, type_list<Head>,
        typename _first_source_impl<type_list<Tail...>, Remaining>::type>;
};

template <typename Remaining>
using _first_source_t = typename _first_source_impl<Remaining, Remaining>::type;

template <typename Remaining>
struct _select_current_layer {
    using parallel = _parallel_source_set_t<Remaining>;
    using type     = std::conditional_t<
            !is_empty_template_v<parallel>, parallel, _first_source_t<Remaining>>;
};

template <typename Remaining>
using _select_current_layer_t = typename _select_current_layer<Remaining>::type;

template <typename Remaining>
struct _layer_graph;

template <typename Remaining>
struct _layer_graph_nonempty;

template <bool HasLayer, typename Remaining>
struct _layer_graph_impl;

template <typename Remaining>
struct _layer_graph_impl<false, Remaining> {
    static constexpr bool value = false;
    using type                  = type_list<>;
};

template <typename Remaining>
struct _layer_graph_impl<true, Remaining> : _layer_graph_nonempty<Remaining> {};

template <template <typename...> typename Template>
struct _layer_graph<Template<>> {
    static constexpr bool value = true;
    using type                  = type_list<>;
};

template <template <typename...> typename Template, typename... Infos>
struct _layer_graph<Template<Infos...>> :
    _layer_graph_impl<
        !is_empty_template_v<_select_current_layer_t<Template<Infos...>>>,
        Template<Infos...>> {};

template <template <typename...> typename Template, typename... Infos>
struct _layer_graph_nonempty<Template<Infos...>> {
    using current_layer = _select_current_layer_t<Template<Infos...>>;
    using remaining = type_list_erase_in_t<Template<Infos...>, current_layer>;
    using next      = _layer_graph<remaining>;

    static constexpr bool value = next::value;
    using type = type_list_cat_t<type_list<current_layer>, typename next::type>;
};

template <typename FullList, typename NodeList>
struct _node_index_list;

template <
    typename FullList, template <typename...> typename Template,
    typename... Nodes>
struct _node_index_list<FullList, Template<Nodes...>> {
    using type = value_list<tuple_first_v<Nodes, FullList>...>;
};

template <typename FullList, typename NodeList>
using _node_index_list_t = typename _node_index_list<FullList, NodeList>::type;

template <typename Node, typename Nodes>
struct _direct_predecessor_nodes;

template <
    typename Node, template <typename...> typename Template, typename... Others>
struct _direct_predecessor_nodes<Node, Template<Others...>> {
    template <typename Other>
    using _predicate = std::bool_constant<
        !std::same_as<Node, Other> && _direct_before_v<Other, Node>>;

    using type = type_list_filt_t<_predicate, Template<Others...>>;
};

template <typename Node, typename Nodes>
using _direct_predecessor_nodes_t =
    typename _direct_predecessor_nodes<Node, Nodes>::type;

template <typename Node, typename Nodes>
struct _direct_successor_nodes;

template <
    typename Node, template <typename...> typename Template, typename... Others>
struct _direct_successor_nodes<Node, Template<Others...>> {
    template <typename Other>
    using _predicate = std::bool_constant<
        !std::same_as<Node, Other> && _direct_before_v<Node, Other>>;

    using type = type_list_filt_t<_predicate, Template<Others...>>;
};

template <typename Node, typename Nodes>
using _direct_successor_nodes_t =
    typename _direct_successor_nodes<Node, Nodes>::type;

template <typename Node, typename Nodes>
struct _descendant_nodes;

template <
    typename Node, template <typename...> typename Template, typename... Others>
struct _descendant_nodes<Node, Template<Others...>> {
    template <typename Other>
    using _predicate = std::bool_constant<
        !std::same_as<Node, Other> &&
        _reachable<Node, Other, Template<Others...>>::value>;

    using type = type_list_filt_t<_predicate, Template<Others...>>;
};

template <typename Node, typename Nodes>
using _descendant_nodes_t = typename _descendant_nodes<Node, Nodes>::type;

template <stage Stage, typename Descriptor>
struct stage_graph {
    using sysinfo     = stage_sysinfo<Stage, Descriptor>;
    using traits_list = typename sysinfo::traits_list;
    using layers      = typename _layer_graph<traits_list>::type;
    using type        = _to_staged_t<Stage, layers>;

    static constexpr bool value =
        sysinfo::value && _layer_graph<traits_list>::value;
    static constexpr size_t size = type_list_size_v<traits_list>;

    template <size_t Index>
    using node_t = type_list_element_t<Index, traits_list>;

    template <size_t Index>
    using predecessor_indices = _node_index_list_t<
        traits_list, _direct_predecessor_nodes_t<node_t<Index>, traits_list>>;

    template <size_t Index>
    using successor_indices = _node_index_list_t<
        traits_list, _direct_successor_nodes_t<node_t<Index>, traits_list>>;

    template <size_t Index>
    using descendant_indices = _node_index_list_t<
        traits_list, _descendant_nodes_t<node_t<Index>, traits_list>>;

private:
    template <typename Values>
    static consteval size_t _value_list_count() noexcept {
        return value_list_size_v<Values>;
    }

    template <size_t Max, typename Values>
    static consteval auto _make_index_row() noexcept {
        std::array<size_t, Max> row{};
        []<size_t... Is>(
            std::array<size_t, Max>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = value_list_element_v<Is, Values>), 0)... };
        }(row, std::make_index_sequence<value_list_size_v<Values>>{});
        return row;
    }

    static consteval auto _make_predecessor_counts() noexcept {
        std::array<size_t, size> counts{};
        []<size_t... Is>(
            std::array<size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<predecessor_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval auto _make_successor_counts() noexcept {
        std::array<size_t, size> counts{};
        []<size_t... Is>(
            std::array<size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<successor_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval auto _make_descendant_counts() noexcept {
        std::array<size_t, size> counts{};
        []<size_t... Is>(
            std::array<size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<descendant_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval size_t
        _max_count(const std::array<size_t, size>& counts) noexcept {
        size_t result = 0;
        for (size_t count : counts) {
            result = count > result ? count : result;
        }
        return result;
    }

    static consteval auto _make_has_commands() noexcept {
        std::array<bool, size> values{};
        []<size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = node_t<Is>::has_commands), 0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_is_individual() noexcept {
        std::array<bool, size> values{};
        []<size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{
                ((target[Is] = node_t<Is>::execute_traits::is_individual), 0)...
            };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    template <size_t Max>
    static consteval auto _make_successor_matrix() noexcept {
        std::array<std::array<size_t, Max>, size> matrix{};
        []<size_t... Is>(
            std::array<std::array<size_t, Max>, size>& target,
            std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _make_index_row<Max, successor_indices<Is>>()),
                0)... };
        }(matrix, std::make_index_sequence<size>{});
        return matrix;
    }

    template <size_t Max>
    static consteval auto _make_descendant_matrix() noexcept {
        std::array<std::array<size_t, Max>, size> matrix{};
        []<size_t... Is>(
            std::array<std::array<size_t, Max>, size>& target,
            std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _make_index_row<Max, descendant_indices<Is>>()),
                0)... };
        }(matrix, std::make_index_sequence<size>{});
        return matrix;
    }

    static consteval size_t
        _count_commands(const std::array<bool, size>& values) noexcept {
        size_t result = 0;
        for (bool current : values) {
            result += current ? 1U : 0U;
        }
        return result;
    }

public:
    static constexpr auto predecessor_counts = _make_predecessor_counts();
    static constexpr auto successor_counts   = _make_successor_counts();
    static constexpr auto descendant_counts  = _make_descendant_counts();

    static constexpr size_t max_successor_count = _max_count(successor_counts);
    static constexpr size_t max_descendant_count =
        _max_count(descendant_counts);

    static constexpr auto has_commands  = _make_has_commands();
    static constexpr auto is_individual = _make_is_individual();
    static constexpr auto successors =
        _make_successor_matrix<max_successor_count>();
    static constexpr auto descendants =
        _make_descendant_matrix<max_descendant_count>();

    static constexpr size_t command_node_count = _count_commands(has_commands);
};

template <stage Stage, typename Descriptor>
using stage_graph_t = typename stage_graph<Stage, Descriptor>::type;

template <typename Descriptor>
struct descriptor_graph {
    using prestartups  = stage_graph_t<pre_startup, Descriptor>;
    using startups     = stage_graph_t<startup, Descriptor>;
    using poststartups = stage_graph_t<post_startup, Descriptor>;

    using firsts  = stage_graph_t<first, Descriptor>;
    using eventss = stage_graph_t<events, Descriptor>;

    using preupdates  = stage_graph_t<pre_update, Descriptor>;
    using updates     = stage_graph_t<update, Descriptor>;
    using postupdates = stage_graph_t<post_update, Descriptor>;

    using renders = stage_graph_t<render, Descriptor>;

    using lasts     = stage_graph_t<last, Descriptor>;
    using shutdowns = stage_graph_t<shutdown, Descriptor>;

    using all = type_list<
        prestartups, startups, poststartups, firsts, eventss, preupdates,
        updates, postupdates, renders, lasts, shutdowns>;

    static constexpr bool value =
        stage_graph<pre_startup, Descriptor>::value &&
        stage_graph<startup, Descriptor>::value &&
        stage_graph<post_startup, Descriptor>::value &&
        stage_graph<first, Descriptor>::value &&
        stage_graph<events, Descriptor>::value &&
        stage_graph<pre_update, Descriptor>::value &&
        stage_graph<update, Descriptor>::value &&
        stage_graph<post_update, Descriptor>::value &&
        stage_graph<render, Descriptor>::value &&
        stage_graph<last, Descriptor>::value &&
        stage_graph<shutdown, Descriptor>::value;
};

template <typename Descriptor>
using descriptor_graph_t = descriptor_graph<Descriptor>;

template <typename SysTraitsList>
struct _descriptor_locals;

template <template <typename...> typename Template, typename... SysTraits>
struct _descriptor_locals<Template<SysTraits...>> {
    template <typename Ty>
    using _has_local = std::negation<internal::_is_empty_sys_tuple<Ty>>;

    using type =
        type_list_filt_t<_has_local, type_list<typename SysTraits::local...>>;
};

template <typename SysTraitsList>
struct _descriptor_queries;

template <template <typename...> typename Template, typename... SysTraits>
struct _descriptor_queries<Template<SysTraits...>> {
    template <typename Ty>
    using _has_query = std::negation<internal::_is_empty_query_tuple<Ty>>;

    using type = type_list_filt_t<
        _has_query, type_list<typename SysTraits::query_cache...>>;
};

template <typename SysTraitsList>
struct _descriptor_resources;

template <template <typename...> typename Template, typename... SysTraits>
struct _descriptor_resources<Template<SysTraits...>> {
    template <typename Ty>
    struct _remove_cvref {
        using type = std::remove_cvref_t<Ty>;
    };

    using type = unique_type_list_t<type_list_convert_t<
        _remove_cvref, type_list_cat_t<typename SysTraits::resource_list...>>>;
};

template <typename SysTraitsList>
struct _descriptor_globals;

template <template <typename...> typename Template, typename... SysTraits>
struct _descriptor_globals<Template<SysTraits...>> {
    template <typename Ty>
    struct _remove_cvref {
        using type = std::remove_cvref_t<Ty>;
    };

    using type = unique_type_list_t<type_list_convert_t<
        _remove_cvref, type_list_cat_t<typename SysTraits::global_list...>>>;
};

template <typename Traits>
struct _to_component_list {
    using type = typename Traits::component_list;
};

template <typename Descriptor>
struct descriptor_traits {

    using sysinfo    = sysinfo_holder<Descriptor>;
    using sys_traits = typename _descriptor_sys_traits<Descriptor>::all;
    using components = type_list_convert_t<
        std::remove_cvref,
        type_list_expose_t<
            type_list, type_list_list_cat_t<type_list_convert_t<
                           _to_component_list, sys_traits>>>>;
    using graph     = descriptor_graph<Descriptor>;
    using events    = events_info<Descriptor>;
    using render    = render_info<Descriptor>;
    using grouped   = graph;
    using runlists  = graph;
    using locals    = typename _descriptor_locals<sys_traits>::type;
    using queries   = typename _descriptor_queries<sys_traits>::type;
    using resources = typename _descriptor_resources<sys_traits>::type;
    using globals   = typename _descriptor_globals<sys_traits>::type;

    static constexpr bool value =
        graph::value && events::value && render::value;
};

} // namespace _metainfo

using _metainfo::descriptor_graph;
using _metainfo::descriptor_graph_t;
using _metainfo::descriptor_traits;
using _metainfo::stage_graph;
using _metainfo::stage_graph_t;

} // namespace neutron
