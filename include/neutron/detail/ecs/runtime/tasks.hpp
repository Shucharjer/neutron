#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"
#include "neutron/detail/ecs/params/construct.hpp"
#include "neutron/detail/ecs/params/construct/local.hpp"
#include "neutron/detail/ecs/params/construct/query.hpp"
#include "neutron/detail/ecs/params/global.hpp"
#include "neutron/detail/ecs/runtime/conditional_sender.hpp"

namespace neutron {

using _time_point_t = std::chrono::system_clock::time_point;

namespace internal {

template <bool Enabled>
struct task_clock {
    _time_point_t last{};
    bool started = false;

    bool
        due(_time_point_t time,
            std::chrono::duration<double> interval) const noexcept {
        return !started || interval.count() == 0.0 || time - last >= interval;
    }

    void complete(_time_point_t time) noexcept {
        last    = time;
        started = true;
    }
};

template <>
struct task_clock<false> {
    bool due(_time_point_t, std::chrono::duration<double>) const noexcept {
        return true;
    }
    void complete(_time_point_t) noexcept {}
};

template <int Rate>
struct task_tick_rate : task_clock<Rate != 0> {
    static constexpr auto interval() noexcept {
        if constexpr (Rate == 0) {
            return std::chrono::duration<double>::zero();
        } else {
            return std::chrono::duration<double>{ 1.0 / Rate };
        }
    }
};

template <int Rate>
requires(Rate == 0)
struct task_tick_rate<Rate> : task_clock<false> {
    static constexpr auto interval() noexcept {
        return std::chrono::duration<double>::zero();
    }
};

template <int Rate>
requires(Rate < 0)
struct task_tick_rate<Rate> : task_clock<true> {
    int rate = 120;
    std::chrono::duration<double> duration{ 1.0 / 120.0 };

    auto interval() const noexcept { return duration; }

    void set_tick_rate(int value) {
        if (value < 0) {
            throw std::invalid_argument("tick rate must be nonnegative");
        }
        rate = value;
        duration =
            std::chrono::duration<double>{ value == 0 ? 0.0 : 1.0 / value };
    }
};

template <typename Requirement>
struct task_requirement {
    static constexpr bool supported     = false;
    static constexpr bool is_individual = false;
    static constexpr bool has_tick_rate = false;
    static constexpr int tick_rate      = 0;
};

template <auto... Functions>
struct task_requirement<_before_t<Functions...>> : task_requirement<void> {
    static constexpr bool supported = true;
};

template <auto... Functions>
struct task_requirement<_after_t<Functions...>> : task_requirement<void> {
    static constexpr bool supported = true;
};

template <>
struct task_requirement<_individual_t> : task_requirement<void> {
    static constexpr bool supported     = true;
    static constexpr bool is_individual = true;
};

template <int Rate>
struct task_requirement<_tick_rate_t<Rate>> : task_requirement<void> {
    static constexpr bool supported     = true;
    static constexpr bool has_tick_rate = true;
    static constexpr int tick_rate      = Rate;
};

template <typename Spec>
struct task_policy;

template <typename Function, auto... Requirements>
struct task_policy<system_spec<Function, Requirements...>> {
    template <auto Requirement>
    using requirement =
        task_requirement<std::remove_cvref_t<decltype(Requirement)>>;

    static_assert(
        (requirement<Requirements>::supported && ...),
        "unsupported system_spec requirement");
    static_assert(
        (0 + ... + requirement<Requirements>::is_individual) <= 1,
        "individual may only be specified once per system");
    static_assert(
        (0 + ... + requirement<Requirements>::has_tick_rate) <= 1,
        "tick_rate may only be specified once per system");

    static constexpr bool individual =
        (false || ... || requirement<Requirements>::is_individual);
    static constexpr int tick_rate =
        (0 + ... + requirement<Requirements>::tick_rate);
};

template <typename Tasks>
struct task_plan;

template <stage Stage, system_spec... Specs>
struct task_plan<_add_systems_t<Stage, Specs...>> {
    using specs                        = value_list<Specs...>;
    static constexpr std::size_t count = sizeof...(Specs);
    static constexpr std::array<bool, count> individual{
        task_policy<decltype(Specs)>::individual...
    };
    static constexpr std::array<int, count> tick_rates{
        task_policy<decltype(Specs)>::tick_rate...
    };

    static constexpr auto levels = []() consteval {
        constexpr auto graph =
            graph_builder<_add_systems_t<Stage, Specs...>>::make();
        std::array<std::size_t, count> result{};
        result.fill(count);
        std::size_t remaining = count;
        for (std::size_t level = 0; level < count && remaining != 0; ++level) {
            for (std::size_t task = 0; task < count; ++task) {
                if (result[task] != count) {
                    continue;
                }
                bool ready = true;
                for (auto dependency : graph[task]) {
                    ready &= result[dependency] < level;
                }
                if (ready) {
                    result[task] = level;
                    --remaining;
                }
            }
        }
        return result;
    }();

    static_assert(
        [] {
            for (auto level : levels) {
                if (level == count) {
                    return false;
                }
            }
            return true;
        }(),
        "system_spec dependencies contain a cycle");

    static constexpr std::size_t level_count = [] {
        std::size_t result = 0;
        for (auto level : levels) {
            if (result <= level) {
                result = level + 1;
            }
        }
        return result;
    }();

    static constexpr auto order = [] {
        std::array<std::size_t, count> result{};
        std::size_t position = 0;
        for (std::size_t level = 0; level < level_count; ++level) {
            for (std::size_t task = 0; task < count; ++task) {
                if (levels[task] == level) {
                    result[position++] = task;
                }
            }
        }
        return result;
    }();

    static constexpr bool has_individual =
        (false || ... || task_policy<decltype(Specs)>::individual);

    template <std::size_t Level, bool Individual>
    static consteval auto indices() {
        inplace_vector<std::size_t, count> result;
        for (std::size_t task = 0; task < count; ++task) {
            if (levels[task] == Level && individual[task] == Individual) {
                result.push_back(task);
            }
        }
        return result;
    }
};

template <
    typename Descriptor, stage Stage,
    typename Tasks = decltype(get_systems<Stage>(Descriptor{}))>
struct stage_tasks;

template <typename Descriptor, stage Stage, system_spec... Specs>
struct stage_tasks<Descriptor, Stage, _add_systems_t<Stage, Specs...>> {
    using plan = task_plan<_add_systems_t<Stage, Specs...>>;
    static constexpr int world_rate = get_forward_tick_rate(Descriptor{});
    ATOM_NO_UNIQUE_ADDR task_clock<
        plan::count != 0 && is_hifreq_stage(Stage) && world_rate != 0>
        clock;
    ATOM_NO_UNIQUE_ADDR std::tuple<task_tick_rate<
        is_hifreq_stage(Stage) ? task_policy<decltype(Specs)>::tick_rate
                               : 0>...>
        clocks;

    template <std::size_t Index, typename World>
    void invoke(World& world, _time_point_t time) {
        auto& timer = std::get<Index>(clocks);
        if (!timer.due(time, timer.interval())) {
            return;
        }
        static constexpr auto spec =
            value_list_element_v<Index, typename plan::specs>;
        [&world]<typename... Args>(type_list<Args...>) {
            auto make_param = [&world]<typename Param>() -> decltype(auto) {
                if constexpr (_is_global<Param>::value) {
                    return Param{};
                } else {
                    return construct_from_world<Stage, spec.fn, Param>(world);
                }
            };
            static_cast<void>(
                spec.fn(make_param.template operator()<Args>()...));
        }(typename decltype(spec)::fn_args{});
        timer.complete(time);
    }

    template <
        std::size_t... Indices, typename World, execution::scheduler Scheduler>
    auto run_group(
        World& world, Scheduler scheduler, _time_point_t time,
        std::index_sequence<Indices...>) {
        if constexpr (sizeof...(Indices) == 0) {
            return execution::just();
        } else {
            auto sender = execution::schedule(scheduler) |
                          execution::then([this, &world, time] {
                              (invoke<Indices>(world, time), ...);
                          });
            if constexpr (
                is_hifreq_stage(Stage) &&
                ((plan::tick_rates[Indices] != 0) && ...)) {
                return conditional_sender{
                    (std::get<Indices>(clocks).due(
                         time, std::get<Indices>(clocks).interval()) ||
                     ...),
                    std::move(sender)
                };
            } else {
                return sender;
            }
        }
    }

    template <std::size_t Index, typename World, execution::scheduler Scheduler>
    auto run_individual(World& world, Scheduler scheduler, _time_point_t time) {
        auto& timer = std::get<Index>(clocks);
        return conditional_sender{ timer.due(time, timer.interval()),
                                   execution::schedule(scheduler) |
                                       execution::then([this, &world, time] {
                                           invoke<Index>(world, time);
                                       }) };
    }

    template <
        std::size_t Level = 0, typename World, execution::scheduler Scheduler>
    auto run(World& world, Scheduler scheduler, _time_point_t time) {
        if constexpr (Level == plan::level_count) {
            return execution::just();
        } else if constexpr (!plan::has_individual) {
            return
                [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
                    return run_group(
                        world, scheduler, time,
                        std::index_sequence<plan::order[Indices]...>{});
                }(std::make_index_sequence<plan::count>{});
        } else {
            static constexpr auto tasks = plan::template indices<Level, true>();
            static constexpr auto grouped =
                plan::template indices<Level, false>();
            auto group =
                [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
                    return run_group(
                        world, scheduler, time,
                        std::index_sequence<grouped[Indices]...>{});
                }(std::make_index_sequence<grouped.size()>{});
            auto batch = [&]<std::size_t... Indices>(
                             std::index_sequence<Indices...>) {
                return execution::when_all(
                    std::move(group),
                    run_individual<tasks[Indices]>(world, scheduler, time)...);
            }(std::make_index_sequence<tasks.size()>{});
            if constexpr (Level + 1 == plan::level_count) {
                return batch;
            } else {
                return std::move(batch) |
                       execution::let_value([this, &world, scheduler, time] {
                           return run<Level + 1>(world, scheduler, time);
                       });
            }
        }
    }
};

template <typename Descriptor, std::size_t... Indices>
auto make_stage_tasks(std::index_sequence<Indices...>)
    -> std::tuple<stage_tasks<Descriptor, static_cast<stage>(Indices)>...>;

template <typename Descriptor>
using world_tasks = decltype(make_stage_tasks<Descriptor>(
    std::make_index_sequence<static_cast<std::size_t>(stage::shutdown) + 1>{}));

} // namespace internal
} // namespace neutron
