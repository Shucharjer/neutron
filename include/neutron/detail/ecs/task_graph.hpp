// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <array>
#include <cstddef>
#include "neutron/detail/ecs/metainfo/graph.hpp"

namespace neutron {

template <typename StageGraph>
class stage_task_graph {
    static constexpr size_t _size = StageGraph::size;

public:
    struct node_state {
        size_t pending_predecessors = 0;
        bool blocked_by_commands = false;
        bool completed = false;
    };

    constexpr stage_task_graph() noexcept { reset(); }

    constexpr void reset() noexcept {
        for (size_t index = 0; index < _size; ++index) {
            nodes_[index].pending_predecessors = StageGraph::predecessor_counts[index];
            nodes_[index].blocked_by_commands = false;
            nodes_[index].completed = false;
        }
        completed_count_ = 0;
        dirty_commands_ = false;
    }

    [[nodiscard]] static constexpr size_t size() noexcept { return _size; }

    [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

    [[nodiscard]] constexpr bool done() const noexcept {
        return completed_count_ == _size;
    }

    [[nodiscard]] constexpr bool dirty_commands() const noexcept {
        return dirty_commands_;
    }

    [[nodiscard]] constexpr size_t completed_count() const noexcept {
        return completed_count_;
    }

    [[nodiscard]] constexpr const node_state& state(size_t index) const noexcept {
        return nodes_[index];
    }

    [[nodiscard]] constexpr bool has_runnable() const noexcept {
        for (size_t index = 0; index < _size; ++index) {
            if (_runnable(index)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool needs_flush() const noexcept {
        return dirty_commands_ && !done() && !has_runnable();
    }

    constexpr size_t collect_runnable(std::array<size_t, _size>& out) const noexcept {
        size_t count = 0;
        for (size_t index = 0; index < _size; ++index) {
            if (_runnable(index) && !StageGraph::is_individual[index]) {
                out[count++] = index;
            }
        }
        if (count != 0) {
            return count;
        }
        for (size_t index = 0; index < _size; ++index) {
            if (_runnable(index)) {
                out[0] = index;
                return 1;
            }
        }
        return 0;
    }

    constexpr void complete(size_t index) noexcept {
        if (nodes_[index].completed) {
            return;
        }

        nodes_[index].completed = true;
        ++completed_count_;

        for (size_t offset = 0; offset < StageGraph::successor_counts[index]; ++offset) {
            const size_t successor = StageGraph::successors[index][offset];
            auto& state = nodes_[successor];
            if (!state.completed && state.pending_predecessors != 0) {
                --state.pending_predecessors;
            }
        }

        if (StageGraph::has_commands[index]) {
            dirty_commands_ = true;
            for (size_t offset = 0; offset < StageGraph::descendant_counts[index]; ++offset) {
                const size_t descendant = StageGraph::descendants[index][offset];
                if (!nodes_[descendant].completed) {
                    nodes_[descendant].blocked_by_commands = true;
                }
            }
        }
    }

    constexpr void complete(
        const std::array<size_t, _size>& completed_indices,
        size_t count) noexcept {
        for (size_t index = 0; index < count; ++index) {
            complete(completed_indices[index]);
        }
    }

    constexpr void flush() noexcept {
        if (!dirty_commands_) {
            return;
        }
        dirty_commands_ = false;
        for (size_t index = 0; index < _size; ++index) {
            if (!nodes_[index].completed) {
                nodes_[index].blocked_by_commands = false;
            }
        }
    }

private:
    [[nodiscard]] constexpr bool _runnable(size_t index) const noexcept {
        const auto& state = nodes_[index];
        return !state.completed && !state.blocked_by_commands &&
               state.pending_predecessors == 0;
    }

    std::array<node_state, _size> nodes_{};
    size_t completed_count_ = 0;
    bool dirty_commands_ = false;
};

} // namespace neutron
