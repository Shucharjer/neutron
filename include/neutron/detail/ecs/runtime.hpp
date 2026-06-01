// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include "neutron/detail/ecs/run_env.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/execution.hpp" // IWYU pragma: keep

namespace neutron {

// NOTE: the CPO `get_scheduler` requires const qualifier, but sometimes the
// scheduler provider like `stdexec::run_loop` or `exec::static_thread_pool`
// do not satify.

template <typename Sp>
concept scheduler_provider = requires(Sp& sp) {
    // { execution::get_start_scheduler(sp) } -> execution::scheduler;
    { sp.get_scheduler() } -> execution::scheduler;
};

template <typename Sp>
concept parallelism_scheduler_provider =
    scheduler_provider<Sp> && requires(const Sp& sp) {
        { sp.available_parallelism() } noexcept -> std::unsigned_integral;
    };

template <typename Payload>
concept valid_payload = requires(Payload& payload) {
    { payload.poll_events() } -> std::same_as<bool>;
    { payload.is_running() } -> std::same_as<bool>;
    payload.render_begin();
    payload.render_end();
};

template <
    scheduler_provider Sp, valid_payload Payload, typename Alloc,
    auto... Worlds>
class runtime {
public:
    template <typename Al = Alloc>
    constexpr runtime(Sp& sp, Payload* payload, const Al& alloc = {})
        : sp_(sp), alloc_(alloc) {}

    constexpr int run() {
        using namespace ::neutron::execution;

        // scheduler auto sch = get_scheduler(sp_);
        scheduler auto sch = sp_.get_scheduler();
        run_envs_for<Alloc, Worlds...> envs{ alloc_ };

        if constexpr (parallelism_scheduler_provider<Sp>) {
            if (sp_.available_parallelism() == 1) [[unlikely]] {
                return _run_weak_parallel(envs, sch);
            }
        }

        forward_progress_guarantee guarantee =
            get_forward_progress_guarantee(sch);

        if (guarantee != forward_progress_guarantee::weakly_parallel) {
            return _run(envs, sch);
        }

        return _run_weak_parallel(envs, sch);
    }

private:
    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run_weak_parallel(Envs& envs, Scheduler& scheduler) {
        [&envs]<std::size_t... Is>(std::index_sequence<Is...>) {
            // step until done
        }(std::make_index_sequence<std::tuple_size_v<Envs>>());

        return 0;
    }

    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run(Envs& envs, Scheduler& scheduler) {
        [&envs]<std::size_t... Is>(std::index_sequence<Is...>) {
            // run until done
        }(std::make_index_sequence<std::tuple_size_v<Envs>>());

        return 0;
    }

    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    Sp& sp_; // NOLINT
};

template <
    auto... Worlds, scheduler_provider Sp, valid_payload Payload,
    typename Alloc = std::allocator<std::byte>>
constexpr auto make_runtime(Sp& sp, Payload* payload, const Alloc& alloc = {}) {
    return runtime<Sp, Payload, Alloc>(sp, payload, alloc);
}

} // namespace neutron
