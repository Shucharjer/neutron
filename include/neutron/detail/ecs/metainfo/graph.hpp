// IWYU pragma: private, include "neutron/detail/ecs/metainfo.hpp"
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
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

enum class _stage_graph_count_kind : std::uint8_t {
    concurrency,
    command_buffers
};

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
    static constexpr std::size_t size = type_list_size_v<traits_list>;

    template <std::size_t Index>
    using node_t = type_list_element_t<Index, traits_list>;

    template <std::size_t Index>
    using predecessor_indices = _node_index_list_t<
        traits_list, _direct_predecessor_nodes_t<node_t<Index>, traits_list>>;

    template <std::size_t Index>
    using successor_indices = _node_index_list_t<
        traits_list, _direct_successor_nodes_t<node_t<Index>, traits_list>>;

    template <std::size_t Index>
    using descendant_indices = _node_index_list_t<
        traits_list, _descendant_nodes_t<node_t<Index>, traits_list>>;

private:
    template <typename Values>
    static consteval std::size_t _value_list_count() noexcept {
        return value_list_size_v<Values>;
    }

    template <std::size_t Max, typename Values>
    static consteval auto _make_index_row() noexcept {
        std::array<std::size_t, Max> row{};
        []<std::size_t... Is>(
            std::array<std::size_t, Max>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = value_list_element_v<Is, Values>), 0)... };
        }(row, std::make_index_sequence<value_list_size_v<Values>>{});
        return row;
    }

    static consteval auto _make_predecessor_counts() noexcept {
        std::array<std::size_t, size> counts{};
        []<std::size_t... Is>(
            std::array<std::size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<predecessor_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval auto _make_successor_counts() noexcept {
        std::array<std::size_t, size> counts{};
        []<std::size_t... Is>(
            std::array<std::size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<successor_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval auto _make_descendant_counts() noexcept {
        std::array<std::size_t, size> counts{};
        []<std::size_t... Is>(
            std::array<std::size_t, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _value_list_count<descendant_indices<Is>>()),
                0)... };
        }(counts, std::make_index_sequence<size>{});
        return counts;
    }

    static consteval std::size_t
        _max_count(const std::array<std::size_t, size>& counts) noexcept {
        std::size_t result = 0;
        for (std::size_t count : counts) {
            result = count > result ? count : result;
        }
        return result;
    }

    static consteval auto _make_has_commands() noexcept {
        std::array<bool, size> values{};
        []<std::size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = node_t<Is>::has_commands), 0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_has_buffered_commands() noexcept {
        std::array<bool, size> values{};
        []<std::size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = node_t<Is>::has_buffered_commands), 0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_is_individual() noexcept {
        std::array<bool, size> values{};
        []<std::size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{
                ((target[Is] = node_t<Is>::execute_traits::is_individual), 0)...
            };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_is_locally_individual() noexcept {
        std::array<bool, size> values{};
        []<std::size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = node_t<Is>::is_locally_individual), 0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_uses_interval() noexcept {
        std::array<bool, size> values{};
        []<std::size_t... Is>(
            std::array<bool, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = node_t<Is>::raw_execute_traits::has_interval),
                0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    static consteval auto _make_execution_intervals() noexcept {
        std::array<double, size> values{};
        []<std::size_t... Is>(
            std::array<double, size>& target, std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] =
                     node_t<Is>::raw_execute_traits::execution_interval),
                0)... };
        }(values, std::make_index_sequence<size>{});
        return values;
    }

    template <std::size_t Max>
    static consteval auto _make_successor_matrix() noexcept {
        std::array<std::array<std::size_t, Max>, size> matrix{};
        []<std::size_t... Is>(
            std::array<std::array<std::size_t, Max>, size>& target,
            std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _make_index_row<Max, successor_indices<Is>>()),
                0)... };
        }(matrix, std::make_index_sequence<size>{});
        return matrix;
    }

    template <std::size_t Max>
    static consteval auto _make_descendant_matrix() noexcept {
        std::array<std::array<std::size_t, Max>, size> matrix{};
        []<std::size_t... Is>(
            std::array<std::array<std::size_t, Max>, size>& target,
            std::index_sequence<Is...>) {
            (void)std::initializer_list<int>{ (
                (target[Is] = _make_index_row<Max, descendant_indices<Is>>()),
                0)... };
        }(matrix, std::make_index_sequence<size>{});
        return matrix;
    }

    static consteval std::size_t
        _count_commands(const std::array<bool, size>& values) noexcept {
        std::size_t result = 0;
        for (bool current : values) {
            result += current ? 1U : 0U;
        }
        return result;
    }

    static consteval bool
        _descendant_of(std::size_t node, std::size_t ancestor) noexcept {
        for (std::size_t offset = 0; offset < descendant_counts[ancestor];
             ++offset) {
            if (descendants[ancestor][offset] == node) {
                return true;
            }
        }
        return false;
    }

    template <_stage_graph_count_kind Kind>
    static consteval auto _make_counted_nodes() noexcept {
        std::array<bool, size> values{};

        for (std::size_t index = 0; index < size; ++index) {
            if constexpr (Kind == _stage_graph_count_kind::concurrency) {
                values[index] = !is_locally_individual[index];
            } else {
                values[index] = has_buffered_commands[index] &&
                                !is_locally_individual[index];
            }
        }

        return values;
    }

    template <_stage_graph_count_kind Kind>
    static consteval std::uint32_t _max_counted_antichain() noexcept {
        constexpr auto counted = _make_counted_nodes<Kind>();

        std::array<std::size_t, size> selected{};
        std::uint32_t best = 0;

        auto remaining_count = [&](std::size_t first) consteval {
            std::uint32_t result = 0;
            for (std::size_t index = first; index < size; ++index) {
                result += counted[index] ? 1U : 0U;
            }
            return result;
        };

        auto compatible_with_selected =
            [&](std::size_t candidate, std::size_t selected_count) consteval {
                for (std::size_t index = 0; index < selected_count; ++index) {
                    const auto current = selected[index];
                    if (_descendant_of(candidate, current) ||
                        _descendant_of(current, candidate)) {
                        return false;
                    }
                }
                return true;
            };

        auto visit = [&]<typename Self>(
                         Self&& self, std::size_t index,
                         std::size_t selected_count,
                         std::uint32_t count) consteval -> void {
            if (index == size) {
                best = best < count ? count : best;
                return;
            }

            if (count + remaining_count(index) <= best) {
                return;
            }

            self(self, index + 1, selected_count, count);

            if (counted[index] &&
                compatible_with_selected(index, selected_count)) {
                selected[selected_count] = index;
                self(self, index + 1, selected_count + 1, count + 1);
            }
        };

        visit(visit, 0, 0, 0);

        if constexpr (Kind == _stage_graph_count_kind::command_buffers) {
            if (best == 0 && buffered_command_node_count != 0) {
                return 1;
            }
        }

        return best;
    }

    template <_stage_graph_count_kind Kind>
    static consteval std::uint32_t
        _timely_resource_count(std::size_t index) noexcept {
        if constexpr (Kind == _stage_graph_count_kind::concurrency) {
            return 1;
        } else {
            return has_buffered_commands[index] ? 1U : 0U;
        }
    }

    template <_stage_graph_count_kind Kind>
    static consteval std::uint32_t _max_timely_layer_count() noexcept {
        std::array<std::size_t, size> pending = predecessor_counts;
        std::array<bool, size> completed{};
        std::uint32_t best        = 0;
        std::size_t completed_cnt = 0;

        while (completed_cnt != size) {
            std::array<bool, size> selected{};
            std::uint32_t current = 0;
            bool has_selected     = false;

            for (std::size_t index = 0; index < size; ++index) {
                if (!completed[index] && pending[index] == 0 &&
                    !is_locally_individual[index]) {
                    selected[index] = true;
                    current += _timely_resource_count<Kind>(index);
                    has_selected = true;
                }
            }

            if (!has_selected) {
                for (std::size_t index = 0; index < size; ++index) {
                    if (!completed[index] && pending[index] == 0) {
                        selected[index] = true;
                        current += _timely_resource_count<Kind>(index);
                        has_selected = true;
                        break;
                    }
                }
            }

            if (!has_selected) {
                return best;
            }

            best = best < current ? current : best;

            for (std::size_t index = 0; index < size; ++index) {
                if (!selected[index]) {
                    continue;
                }

                completed[index] = true;
                ++completed_cnt;

                for (std::size_t offset = 0; offset < successor_counts[index];
                     ++offset) {
                    const std::size_t successor = successors[index][offset];
                    if (pending[successor] != 0) {
                        --pending[successor];
                    }
                }
            }
        }

        return best;
    }

public:
    static constexpr auto predecessor_counts = _make_predecessor_counts();
    static constexpr auto successor_counts   = _make_successor_counts();
    static constexpr auto descendant_counts  = _make_descendant_counts();

    static constexpr std::size_t max_successor_count =
        _max_count(successor_counts);
    static constexpr std::size_t max_descendant_count =
        _max_count(descendant_counts);

    static constexpr auto has_commands          = _make_has_commands();
    static constexpr auto has_buffered_commands = _make_has_buffered_commands();
    static constexpr auto is_individual         = _make_is_individual();
    static constexpr auto is_locally_individual = _make_is_locally_individual();
    static constexpr auto uses_interval         = _make_uses_interval();
    static constexpr auto execution_intervals   = _make_execution_intervals();
    static constexpr auto successors =
        _make_successor_matrix<max_successor_count>();
    static constexpr auto descendants =
        _make_descendant_matrix<max_descendant_count>();

    static constexpr std::size_t command_node_count =
        _count_commands(has_commands);
    static constexpr std::size_t buffered_command_node_count =
        _count_commands(has_buffered_commands);
    static constexpr std::size_t interval_task_count =
        _count_commands(uses_interval);
    static constexpr std::uint32_t max_concurrency =
        _max_counted_antichain<_stage_graph_count_kind::concurrency>();
    static constexpr std::uint32_t max_buffer_count =
        _max_counted_antichain<_stage_graph_count_kind::command_buffers>();
    static constexpr std::uint32_t min_concurrency =
        _max_timely_layer_count<_stage_graph_count_kind::concurrency>();
    static constexpr std::uint32_t min_buffer_count =
        _max_timely_layer_count<_stage_graph_count_kind::command_buffers>();
};

template <stage Stage, typename Descriptor>
using stage_graph_t = typename stage_graph<Stage, Descriptor>::type;

template <std::uint32_t... Values>
consteval std::uint32_t _max_graph_value() noexcept {
    std::uint32_t result = 0;
    ((result = result < Values ? Values : result), ...);
    return result;
}

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

private:
    static constexpr std::uint32_t _raw_max_concurrency = _max_graph_value<
        stage_graph<pre_startup, Descriptor>::max_concurrency,
        stage_graph<startup, Descriptor>::max_concurrency,
        stage_graph<post_startup, Descriptor>::max_concurrency,
        stage_graph<first, Descriptor>::max_concurrency,
        stage_graph<events, Descriptor>::max_concurrency,
        stage_graph<pre_update, Descriptor>::max_concurrency,
        stage_graph<update, Descriptor>::max_concurrency,
        stage_graph<post_update, Descriptor>::max_concurrency,
        stage_graph<render, Descriptor>::max_concurrency,
        stage_graph<last, Descriptor>::max_concurrency,
        stage_graph<shutdown, Descriptor>::max_concurrency>();
    static constexpr std::uint32_t _raw_min_concurrency = _max_graph_value<
        stage_graph<pre_startup, Descriptor>::min_concurrency,
        stage_graph<startup, Descriptor>::min_concurrency,
        stage_graph<post_startup, Descriptor>::min_concurrency,
        stage_graph<first, Descriptor>::min_concurrency,
        stage_graph<events, Descriptor>::min_concurrency,
        stage_graph<pre_update, Descriptor>::min_concurrency,
        stage_graph<update, Descriptor>::min_concurrency,
        stage_graph<post_update, Descriptor>::min_concurrency,
        stage_graph<render, Descriptor>::min_concurrency,
        stage_graph<last, Descriptor>::min_concurrency,
        stage_graph<shutdown, Descriptor>::min_concurrency>();

public:
    static constexpr std::uint32_t max_concurrency =
        _raw_max_concurrency == 0 ? 1 : _raw_max_concurrency;
    static constexpr std::uint32_t min_concurrency =
        _raw_min_concurrency == 0 ? 1 : _raw_min_concurrency;
    static constexpr std::uint32_t max_buffer_count = _max_graph_value<
        stage_graph<pre_startup, Descriptor>::max_buffer_count,
        stage_graph<startup, Descriptor>::max_buffer_count,
        stage_graph<post_startup, Descriptor>::max_buffer_count,
        stage_graph<first, Descriptor>::max_buffer_count,
        stage_graph<events, Descriptor>::max_buffer_count,
        stage_graph<pre_update, Descriptor>::max_buffer_count,
        stage_graph<update, Descriptor>::max_buffer_count,
        stage_graph<post_update, Descriptor>::max_buffer_count,
        stage_graph<render, Descriptor>::max_buffer_count,
        stage_graph<last, Descriptor>::max_buffer_count,
        stage_graph<shutdown, Descriptor>::max_buffer_count>();
    static constexpr std::uint32_t min_buffer_count = _max_graph_value<
        stage_graph<pre_startup, Descriptor>::min_buffer_count,
        stage_graph<startup, Descriptor>::min_buffer_count,
        stage_graph<post_startup, Descriptor>::min_buffer_count,
        stage_graph<first, Descriptor>::min_buffer_count,
        stage_graph<events, Descriptor>::min_buffer_count,
        stage_graph<pre_update, Descriptor>::min_buffer_count,
        stage_graph<update, Descriptor>::min_buffer_count,
        stage_graph<post_update, Descriptor>::min_buffer_count,
        stage_graph<render, Descriptor>::min_buffer_count,
        stage_graph<last, Descriptor>::min_buffer_count,
        stage_graph<shutdown, Descriptor>::min_buffer_count>();
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
