#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include "neutron/detail/execution/start_detached.hpp"
#include "neutron/execution.hpp"
#include "neutron/inplace_vector.hpp"
// #include "neutron/stop_token.hpp"

namespace neutron {

struct _task_base {
    void (*const task)()         = nullptr; // NOLINT
    std::atomic<bool>* completed = nullptr;

    void execute() const {
        task();
        completed->store(true, std::memory_order_release);
    }
};

template <typename Tasks>
class task_executor {
public:
    static constexpr std::size_t task_count = 0;

    constexpr task_executor() {
        for (std::size_t task = 0; task < task_count; ++task) {
            tasks_[task].completed = &completed_[task]; // NOLINT
        }
    }

    constexpr bool done() noexcept {
        return std::ranges::all_of(
            completed_, [](const std::atomic<bool>& completed) noexcept {
                return completed.load(std::memory_order_relaxed);
            });
    }

    constexpr auto collect() -> inplace_vector<_task_base, task_count> {
        // TODO: collect ready tasks and mark them as dispatched
        return {};
    }

private:
    std::array<std::atomic<bool>, task_count> completed_{};
    std::array<bool, task_count> dispatched_{};
    std::array<_task_base, task_count> tasks_{};
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

template <typename Tasks, typename Scheduler>
constexpr void execute_until_done(Scheduler& scheduler) {
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
    task_executor<Tasks> executor;
    while (!executor.done()) {
        auto tasks = executor.collect();
        for (const _task_base& task : tasks) {
            auto sndr = schedule(scheduler) |
                        then([&task]() noexcept { task.execute(); });
            // execution::spawn(std::move(sndr), null_scope_token{});
            ::neutron::execution::start_detached(std::move(sndr));
        }
    }
}

} // namespace neutron
