// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/basic_commands.hpp"
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/ecs/metainfo/graph.hpp"
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/element.hpp"
#include "neutron/detail/metafn/size.hpp"

namespace neutron {

template <stage Stage, typename Descriptor>
struct _world_task_set {
    using descriptor_type = Descriptor;
    using graph_type      = stage_graph<Stage, Descriptor>;

    static constexpr stage stage_value = Stage;
};

template <typename TaskSet>
struct _stage_task_graph {
    using task_set_type   = TaskSet;
    using descriptor_type = typename TaskSet::descriptor_type;
    using graph_type      = typename TaskSet::graph_type;

    static_assert(graph_type::value, "Invalid ECS stage graph");

    static constexpr stage stage_value       = TaskSet::stage_value;
    static constexpr std::size_t size        = graph_type::size;
    static constexpr std::size_t task_count  = size;
    static constexpr std::size_t world_count = 1;

    template <std::size_t Index>
    using node_t = typename graph_type::template node_t<Index>;

private:
    static consteval auto _make_local_task_indices() noexcept {
        std::array<std::size_t, task_count> result{};
        for (std::size_t index = 0; index < task_count; ++index) {
            result[index] = index;
        }
        return result;
    }

    static consteval auto _make_world_indices() noexcept {
        std::array<std::size_t, task_count> result{};
        return result;
    }

public:
    static constexpr auto predecessor_counts = graph_type::predecessor_counts;
    static constexpr auto successor_counts   = graph_type::successor_counts;
    static constexpr auto descendant_counts  = graph_type::descendant_counts;

    static constexpr std::size_t max_successor_count =
        graph_type::max_successor_count;
    static constexpr std::size_t max_descendant_count =
        graph_type::max_descendant_count;

    static constexpr auto successors   = graph_type::successors;
    static constexpr auto descendants  = graph_type::descendants;
    static constexpr auto has_commands = graph_type::has_commands;
    static constexpr auto has_buffered_commands =
        graph_type::has_buffered_commands;
    static constexpr auto is_individual = graph_type::is_individual;
    static constexpr auto is_locally_individual =
        graph_type::is_locally_individual;
    static constexpr auto uses_interval       = graph_type::uses_interval;
    static constexpr auto execution_intervals = graph_type::execution_intervals;
    static constexpr auto world_indices       = _make_world_indices();
    static constexpr auto local_task_indices  = _make_local_task_indices();

    static constexpr std::size_t command_node_count =
        graph_type::command_node_count;
    static constexpr std::size_t buffered_command_node_count =
        graph_type::buffered_command_node_count;
    static constexpr std::size_t interval_task_count =
        graph_type::interval_task_count;
    static constexpr std::uint32_t max_concurrency =
        graph_type::max_concurrency;
    static constexpr std::uint32_t max_buffer_count =
        graph_type::max_buffer_count;
    static constexpr std::uint32_t min_concurrency =
        graph_type::min_concurrency;
    static constexpr std::uint32_t min_buffer_count =
        graph_type::min_buffer_count;
};

template <typename... TaskSets>
requires(sizeof...(TaskSets) != 0)
struct _env_task_graph {
    static constexpr stage stage_value =
        type_list_element_t<0, type_list<TaskSets...>>::stage_value;

    static_assert(
        ((TaskSets::stage_value == stage_value) && ...),
        "all task sets in an environment graph must use the same stage");

    static constexpr std::size_t world_count = sizeof...(TaskSets);
    static constexpr std::size_t task_count =
        (std::size_t{ 0 } + ... + _stage_task_graph<TaskSets>::task_count);

    template <std::size_t Index>
    using task_set = type_list_element_t<Index, type_list<TaskSets...>>;

private:
    template <typename Graph>
    static consteval std::size_t _task_count_of() noexcept {
        return Graph::task_count;
    }

    static consteval auto _make_world_task_counts() noexcept {
        return std::array<std::size_t, world_count>{
            _stage_task_graph<TaskSets>::task_count...
        };
    }

    static consteval auto _make_world_offsets() noexcept {
        std::array<std::size_t, world_count> result{};
        std::size_t offset = 0;
        for (std::size_t world = 0; world < world_count; ++world) {
            result[world] = offset;
            offset += world_task_counts[world];
        }
        return result;
    }

    static consteval std::size_t
        _max_value(std::array<std::size_t, world_count> values) noexcept {
        std::size_t result = 0;
        for (auto value : values) {
            result = result < value ? value : result;
        }
        return result;
    }

    static consteval auto _make_max_successor_counts() noexcept {
        return std::array<std::size_t, world_count>{
            _stage_task_graph<TaskSets>::max_successor_count...
        };
    }

    static consteval auto _make_max_descendant_counts() noexcept {
        return std::array<std::size_t, world_count>{
            _stage_task_graph<TaskSets>::max_descendant_count...
        };
    }

    template <typename TaskSet>
    static consteval void _append_world_indices(
        std::array<std::size_t, task_count>& result, std::size_t world,
        std::size_t& flat) noexcept {
        using graph = _stage_task_graph<TaskSet>;
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            result[flat++] = world;
        }
    }

    static consteval auto _make_world_indices() noexcept {
        std::array<std::size_t, task_count> result{};
        std::size_t flat  = 0;
        std::size_t world = 0;
        ((_append_world_indices<TaskSets>(result, world++, flat)), ...);
        return result;
    }

    template <typename TaskSet>
    static consteval void _append_local_task_indices(
        std::array<std::size_t, task_count>& result,
        std::size_t& flat) noexcept {
        using graph = _stage_task_graph<TaskSet>;
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            result[flat++] = local;
        }
    }

    static consteval auto _make_local_task_indices() noexcept {
        std::array<std::size_t, task_count> result{};
        std::size_t flat = 0;
        ((_append_local_task_indices<TaskSets>(result, flat)), ...);
        return result;
    }

    template <typename TaskSet>
    static consteval void _append_counts(
        std::array<std::size_t, task_count>& target,
        const std::array<std::size_t, _stage_task_graph<TaskSet>::task_count>&
            source,
        std::size_t& flat) noexcept {
        using graph = _stage_task_graph<TaskSet>;
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            target[flat++] = source[local];
        }
    }

    static consteval auto _make_predecessor_counts() noexcept {
        std::array<std::size_t, task_count> result{};
        std::size_t flat = 0;
        ((_append_counts<TaskSets>(
             result, _stage_task_graph<TaskSets>::predecessor_counts, flat)),
         ...);
        return result;
    }

    static consteval auto _make_successor_counts() noexcept {
        std::array<std::size_t, task_count> result{};
        std::size_t flat = 0;
        ((_append_counts<TaskSets>(
             result, _stage_task_graph<TaskSets>::successor_counts, flat)),
         ...);
        return result;
    }

    static consteval auto _make_descendant_counts() noexcept {
        std::array<std::size_t, task_count> result{};
        std::size_t flat = 0;
        ((_append_counts<TaskSets>(
             result, _stage_task_graph<TaskSets>::descendant_counts, flat)),
         ...);
        return result;
    }

    template <
        typename TaskSet, std::size_t Max,
        std::size_t _stage_task_graph<TaskSet>::* = nullptr>
    struct _unused_member_pointer {};

    template <typename TaskSet, std::size_t Max>
    static consteval void _append_successors(
        std::array<std::array<std::size_t, Max>, task_count>& target,
        std::size_t world, std::size_t& flat) noexcept {
        using graph       = _stage_task_graph<TaskSet>;
        const auto offset = world_offsets[world];
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            const auto row = flat++;
            for (std::size_t edge = 0; edge < graph::successor_counts[local];
                 ++edge) {
                target[row][edge] = offset + graph::successors[local][edge];
            }
        }
    }

    template <std::size_t Max>
    static consteval auto _make_successors() noexcept {
        std::array<std::array<std::size_t, Max>, task_count> result{};
        std::size_t flat  = 0;
        std::size_t world = 0;
        ((_append_successors<TaskSets, Max>(result, world++, flat)), ...);
        return result;
    }

    template <typename TaskSet, std::size_t Max>
    static consteval void _append_descendants(
        std::array<std::array<std::size_t, Max>, task_count>& target,
        std::size_t world, std::size_t& flat) noexcept {
        using graph       = _stage_task_graph<TaskSet>;
        const auto offset = world_offsets[world];
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            const auto row = flat++;
            for (std::size_t edge = 0; edge < graph::descendant_counts[local];
                 ++edge) {
                target[row][edge] = offset + graph::descendants[local][edge];
            }
        }
    }

    template <std::size_t Max>
    static consteval auto _make_descendants() noexcept {
        std::array<std::array<std::size_t, Max>, task_count> result{};
        std::size_t flat  = 0;
        std::size_t world = 0;
        ((_append_descendants<TaskSets, Max>(result, world++, flat)), ...);
        return result;
    }

    template <typename TaskSet, typename Value>
    static consteval void _append_values(
        std::array<Value, task_count>& target,
        const std::array<Value, _stage_task_graph<TaskSet>::task_count>& source,
        std::size_t& flat) noexcept {
        using graph = _stage_task_graph<TaskSet>;
        for (std::size_t local = 0; local < graph::task_count; ++local) {
            target[flat++] = source[local];
        }
    }

    static consteval auto _make_has_commands() noexcept {
        std::array<bool, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::has_commands, flat)),
         ...);
        return result;
    }

    static consteval auto _make_has_buffered_commands() noexcept {
        std::array<bool, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::has_buffered_commands, flat)),
         ...);
        return result;
    }

    static consteval auto _make_is_individual() noexcept {
        std::array<bool, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::is_individual, flat)),
         ...);
        return result;
    }

    static consteval auto _make_is_locally_individual() noexcept {
        std::array<bool, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::is_locally_individual, flat)),
         ...);
        return result;
    }

    static consteval auto _make_uses_interval() noexcept {
        std::array<bool, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::uses_interval, flat)),
         ...);
        return result;
    }

    static consteval auto _make_execution_intervals() noexcept {
        std::array<double, task_count> result{};
        std::size_t flat = 0;
        ((_append_values<TaskSets>(
             result, _stage_task_graph<TaskSets>::execution_intervals, flat)),
         ...);
        return result;
    }

    static consteval std::size_t
        _count_true(const std::array<bool, task_count>& values) noexcept {
        std::size_t result = 0;
        for (bool value : values) {
            result += value ? 1U : 0U;
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

    static consteval auto _make_concurrency_counted_nodes() noexcept {
        std::array<bool, task_count> values{};
        for (std::size_t index = 0; index < task_count; ++index) {
            values[index] = !is_locally_individual[index];
        }
        return values;
    }

    static consteval std::uint32_t _max_counted_antichain() noexcept {
        constexpr auto counted = _make_concurrency_counted_nodes();

        std::array<std::size_t, task_count> selected{};
        std::uint32_t best = 0;

        auto remaining_count = [&](std::size_t first) consteval {
            std::uint32_t result = 0;
            for (std::size_t index = first; index < task_count; ++index) {
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
            if (index == task_count) {
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
        return best;
    }

public:
    static constexpr auto world_task_counts = _make_world_task_counts();
    static constexpr auto world_offsets     = _make_world_offsets();

private:
    static constexpr auto _max_successor_counts = _make_max_successor_counts();
    static constexpr auto _max_descendant_counts =
        _make_max_descendant_counts();

public:
    static constexpr std::size_t max_successor_count =
        _max_value(_max_successor_counts);
    static constexpr std::size_t max_descendant_count =
        _max_value(_max_descendant_counts);

    static constexpr auto predecessor_counts = _make_predecessor_counts();
    static constexpr auto successor_counts   = _make_successor_counts();
    static constexpr auto descendant_counts  = _make_descendant_counts();
    static constexpr auto successors = _make_successors<max_successor_count>();
    static constexpr auto descendants =
        _make_descendants<max_descendant_count>();

    static constexpr auto world_indices      = _make_world_indices();
    static constexpr auto local_task_indices = _make_local_task_indices();

    static constexpr auto has_commands          = _make_has_commands();
    static constexpr auto has_buffered_commands = _make_has_buffered_commands();
    static constexpr auto is_individual         = _make_is_individual();
    static constexpr auto is_locally_individual = _make_is_locally_individual();
    static constexpr auto uses_interval         = _make_uses_interval();
    static constexpr auto execution_intervals   = _make_execution_intervals();

    static constexpr std::size_t command_node_count = _count_true(has_commands);
    static constexpr std::size_t buffered_command_node_count =
        _count_true(has_buffered_commands);
    static constexpr std::size_t interval_task_count =
        _count_true(uses_interval);

    static constexpr std::uint32_t max_concurrency = _max_counted_antichain();
    static constexpr std::uint32_t max_buffer_count =
        (std::uint32_t{ 0 } + ... +
         _stage_task_graph<TaskSets>::max_buffer_count);
    static constexpr std::uint32_t min_concurrency =
        (std::uint32_t{ 0 } + ... +
         _stage_task_graph<TaskSets>::min_concurrency);
    static constexpr std::uint32_t min_buffer_count =
        (std::uint32_t{ 0 } + ... +
         _stage_task_graph<TaskSets>::min_buffer_count);
};

template <typename EnvGraph>
class _env_task_executor {
    static constexpr std::size_t _task_count  = EnvGraph::task_count;
    static constexpr std::size_t _world_count = EnvGraph::world_count;

    enum _task_state : unsigned char {
        _completed = 1U << 0U,
        _running   = 1U << 1U,
        _blocked   = 1U << 2U,
    };

public:
    constexpr _env_task_executor() noexcept { reset(); }

    constexpr void reset(
        const std::array<bool, _world_count>* active_worlds =
            nullptr) noexcept {
        completed_count_ = 0;
        running_by_world_.fill(0);
        dirty_worlds_.fill(false);

        for (std::size_t world = 0; world < _world_count; ++world) {
            active_worlds_[world] =
                active_worlds == nullptr || (*active_worlds)[world];
        }

        for (std::size_t task = 0; task < _task_count; ++task) {
            pending_predecessors_[task] = EnvGraph::predecessor_counts[task];
            state_[task]                = 0;
        }

        for (std::size_t task = 0; task < _task_count; ++task) {
            if (!active_worlds_[EnvGraph::world_indices[task]]) {
                skip(task);
            }
        }
    }

    [[nodiscard]] constexpr bool done() const noexcept {
        return completed_count_ == _task_count;
    }

    [[nodiscard]] constexpr bool running(std::size_t task) const noexcept {
        return (state_[task] & _running) != 0;
    }

    [[nodiscard]] constexpr bool completed(std::size_t task) const noexcept {
        return (state_[task] & _completed) != 0;
    }

    [[nodiscard]] constexpr bool blocked(std::size_t task) const noexcept {
        return (state_[task] & _blocked) != 0;
    }

    [[nodiscard]] constexpr std::size_t completed_count() const noexcept {
        return completed_count_;
    }

    [[nodiscard]] constexpr std::size_t
        running_count(std::size_t world) const noexcept {
        return running_by_world_[world];
    }

    [[nodiscard]] constexpr bool dirty(std::size_t world) const noexcept {
        return dirty_worlds_[world];
    }

    [[nodiscard]] constexpr bool runnable(std::size_t task) const noexcept {
        return active_worlds_[EnvGraph::world_indices[task]] &&
               ((state_[task] & (_completed | _running | _blocked)) == 0) &&
               pending_predecessors_[task] == 0;
    }

    [[nodiscard]] constexpr bool
        has_runnable(std::size_t world) const noexcept {
        for (std::size_t task = 0; task < _task_count; ++task) {
            if (EnvGraph::world_indices[task] == world && runnable(task)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool needs_flush(std::size_t world) const noexcept {
        return dirty_worlds_[world] && running_by_world_[world] == 0 &&
               !has_runnable(world);
    }

    template <std::size_t Capacity>
    constexpr std::size_t collect_runnable(
        std::array<std::size_t, Capacity>& out) const noexcept {
        std::size_t count = 0;
        for (std::size_t task = 0; task < _task_count; ++task) {
            if (runnable(task) && !EnvGraph::is_individual[task]) {
                out[count++] = task;
            }
        }
        if (count != 0) {
            return count;
        }
        for (std::size_t task = 0; task < _task_count; ++task) {
            if (runnable(task)) {
                out[0] = task;
                return 1;
            }
        }
        return 0;
    }

    constexpr void mark_running(std::size_t task) noexcept {
        state_[task] |= _running;
        ++running_by_world_[EnvGraph::world_indices[task]];
    }

    constexpr void complete(std::size_t task) noexcept {
        if ((state_[task] & _completed) != 0) {
            return;
        }

        state_[task] &= static_cast<unsigned char>(~_running);
        state_[task] |= _completed;
        --running_by_world_[EnvGraph::world_indices[task]];
        ++completed_count_;

        for (std::size_t offset = 0; offset < EnvGraph::successor_counts[task];
             ++offset) {
            const auto successor = EnvGraph::successors[task][offset];
            if (pending_predecessors_[successor] != 0) {
                --pending_predecessors_[successor];
            }
        }

        if (EnvGraph::has_commands[task]) {
            const auto world     = EnvGraph::world_indices[task];
            dirty_worlds_[world] = true;
            for (std::size_t offset = 0;
                 offset < EnvGraph::descendant_counts[task]; ++offset) {
                const auto descendant = EnvGraph::descendants[task][offset];
                if ((state_[descendant] & _completed) == 0) {
                    state_[descendant] |= _blocked;
                }
            }
        }
    }

    constexpr void skip(std::size_t task) noexcept {
        if ((state_[task] & _completed) != 0) {
            return;
        }

        state_[task] &= static_cast<unsigned char>(~_running);
        state_[task] |= _completed;
        ++completed_count_;

        for (std::size_t offset = 0; offset < EnvGraph::successor_counts[task];
             ++offset) {
            const auto successor = EnvGraph::successors[task][offset];
            if (pending_predecessors_[successor] != 0) {
                --pending_predecessors_[successor];
            }
        }
    }

    constexpr void flush(std::size_t world) noexcept {
        if (!dirty_worlds_[world]) {
            return;
        }

        dirty_worlds_[world] = false;
        for (std::size_t task = 0; task < _task_count; ++task) {
            if (EnvGraph::world_indices[task] == world &&
                (state_[task] & _completed) == 0) {
                state_[task] &= static_cast<unsigned char>(~_blocked);
            }
        }
    }

private:
    std::array<std::size_t, _task_count> pending_predecessors_{};
    std::array<unsigned char, _task_count> state_{};
    std::array<std::size_t, _world_count> running_by_world_{};
    std::array<bool, _world_count> dirty_worlds_{};
    std::array<bool, _world_count> active_worlds_{};
    std::size_t completed_count_ = 0;
};

namespace _task_graph {

template <auto Sys, typename Arg, std::size_t Index, typename World>
decltype(auto) _make_arg(
    World& world, typename std::remove_cvref_t<World>::command_buffer* cmdbuf) {
    using world_type    = std::remove_cvref_t<World>;
    using command_type  = typename world_type::command_buffer;
    using command_alloc = typename command_type::allocator_type;
    using arg_type      = std::remove_cvref_t<Arg>;

    if constexpr (std::same_as<
                      arg_type, basic_commands<command_alloc, false>>) {
        assert(cmdbuf != nullptr);
        return basic_commands<command_alloc, false>{ *cmdbuf };
    } else if constexpr (std::same_as<
                             arg_type, basic_commands<command_alloc, true>>) {
        return basic_commands<command_alloc, true>{ world };
    } else {
        return construct_from_world<Sys, Arg>(world);
    }
}

template <typename Node, std::size_t Index, typename ArgList>
struct _node_invoker;

template <
    typename Node, std::size_t Index, template <typename...> typename Template,
    typename... Args>
struct _node_invoker<Node, Index, Template<Args...>> {
    template <world World>
    static void call(
        World& world,
        typename std::remove_cvref_t<World>::command_buffer* cmdbuf) {
        Node::fn(_make_arg<Node::fn, Args, Index>(world, cmdbuf)...);
    }
};

template <typename TaskSet, std::size_t Index>
struct _task_invoker {
    using graph_type = _stage_task_graph<TaskSet>;
    using node_type  = typename graph_type::template node_t<Index>;

    template <world World>
    static void call(
        World& world,
        typename std::remove_cvref_t<World>::command_buffer* cmdbuf) {
        _node_invoker<node_type, Index, typename node_type::arg_list>::call(
            world, cmdbuf);
    }
};

} // namespace _task_graph

} // namespace neutron
