#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include "neutron/detail/ecs/descriptor.hpp"
#include "neutron/detail/execution/start_detached.hpp"
#include "neutron/execution.hpp"
#include "neutron/inplace_vector.hpp"
#include "neutron/detail/ecs/fwd.hpp"
// #include "neutron/stop_token.hpp"

namespace neutron {

template <stage, typename>
struct tasks_of_impl;

template <stage, typename>
struct _get_tasks;
template <stage Stage>
struct _get_tasks<Stage, type_list<>> {
    using type = _add_systems_t<Stage>;
};
template <stage Stage, auto... Desc, typename... Others>
struct _get_tasks<Stage, type_list<_add_systems_t<Stage, Desc...>, Others...>> {
    using type = _add_systems_t<Stage, Desc...>;
};
template <stage Stage, typename Ty, typename... Others>
struct _get_tasks<Stage, type_list<Ty, Others...>> {
    using type = typename _get_tasks<Stage, type_list<Others...>>::type;
};

template <stage Stage, typename... Args>
struct tasks_of_impl<Stage, world_descriptor_t<Args...>> {
    using type = typename _get_tasks<Stage, type_list<Args...>>::type;
};

template <stage Stage, typename Specs>
using tasks_of = typename tasks_of_impl<Stage, Specs>::type;

template <typename Alloc>
struct task_node {
    void (*const _task)(world_base<Alloc>&) = nullptr; // NOLINT
    std::atomic<bool>* _done                = nullptr;

    void execute(world_base<Alloc>& world) {
        _task(world);
        _done->store(true, std::memory_order_release);
    }
};

template <auto Fn, typename = decltype(Fn)>
struct task_invoker;
template <auto Fn, typename... Args>
struct task_invoker<Fn, void (*)(Args...)> {
    template <typename World>
    void operator()(World& world) {
        Fn(Args(world)...);
    }
};
template <auto Fn, typename... Args>
struct task_invoker<Fn, void (*)(Args...) noexcept> {
    template <typename World>
    void operator()(World& world) noexcept {
        Fn(Args(world)...);
    }
};

template <auto Fn, typename = decltype(Fn)>
constexpr bool _is_nothrow_invocable_function = false;
template <auto Fn, typename Ret, typename... Args>
constexpr bool _is_nothrow_invocable_function<Fn, Ret (*)(Args...) noexcept> =
    true;

template <typename World, system_spec Spec>
struct task;
template <typename Descriptor, typename Alloc, system_spec Spec>
struct task<basic_world<Descriptor, Alloc>, Spec> : task_node<Alloc> {
    static void execute_fn(world_base<Alloc>& base) noexcept(
        _is_nothrow_invocable_function<Spec.fn>) {
        auto& world = static_cast<basic_world<Descriptor, Alloc>&>(base);
        task_invoker<Spec.fn>()(world);
    }

    consteval task() noexcept : task_node<Alloc>{ .content = execute_fn } {}
};

template <typename Tasks, typename Alloc>
class executor;

template <typename Alloc>
struct task_context {
    task_node<Alloc>* task;
    world_base<Alloc>* world;
};

template <system_spec... Specs, typename Alloc>
class executor<value_list<Specs...>, Alloc> {
    using context_t = task_context<Alloc>;

    static constexpr std::size_t count = sizeof...(Specs);

public:
    using task_node_t = task_node<Alloc>;

    executor() = delete;

    executor(const executor&)            = delete;
    executor(executor&&)                 = delete;
    executor& operator=(const executor&) = delete;
    executor& operator=(executor&&)      = delete;

    ~executor() = default;

    bool done() noexcept {
        return std::ranges::all_of(done_, [](const std::atomic<bool>& flag) {
            return flag.load(std::memory_order_relaxed);
        });
    }

    auto collect() -> inplace_vector<context_t, count> {
        inplace_vector<context_t, count> collection;
        for (std::size_t i = 0; i < count; ++i) {
            if (done_[i].load(std::memory_order_acquire) || dispatched_[i]) {
                continue;
            }

            collection.emplace_back(tasks_[i], worlds_[i]);
            dispatched_[i] = true;
        }
        return collection;
    }

private:
    inplace_vector<std::atomic<bool>, count> done_;
    inplace_vector<bool, count> dispatched_;
    inplace_vector<task_node<Alloc>, count> tasks_;
    inplace_vector<world_base<Alloc>*, count> worlds_;
};

// struct null_scope_token {
//     struct assoc {
//         constexpr explicit operator bool() const noexcept { return true; }

//         // NOLINTNEXTLINE
//         ATOM_NODISCARD constexpr assoc try_associate() const noexcept {
//             return {};
//         }
//     };

//     // NOLINTNEXTLINE
//     constexpr auto try_associate() const noexcept -> assoc { return {}; }

//     template <execution::sender Sndr>
//     constexpr execution::sender auto wrap(Sndr&& sndr) const noexcept {
//         return std::forward<Sndr>(sndr);
//     }
// };

template <typename Executor, typename Scheduler>
constexpr void execute_until_done(Executor& executor, Scheduler& scheduler) {
    // struct _env_t {
    //     ATOM_NODISCARD constexpr auto
    //         query(execution::get_stop_token_t) const noexcept
    //         -> never_stop_token {
    //         return {};
    //     }

    //     ATOM_NODISCARD constexpr auto
    //         query(execution::get_start_scheduler_t) const noexcept
    //         -> execution::inline_scheduler {
    //         return {};
    //     }
    // };
    using namespace execution;
    using task_node_t = typename std::remove_cvref_t<Executor>::task_node_t;
    while (!executor.done()) {
        auto tasks = executor.collect();
        for (const task_node_t& task : tasks) {
            auto sndr = schedule(scheduler) |
                        then([&task]() noexcept { task.execute(); });
            // execution::spawn(std::move(sndr), null_scope_token{});
            ::neutron::execution::start_detached(std::move(sndr));
        }
    }
}

} // namespace neutron
