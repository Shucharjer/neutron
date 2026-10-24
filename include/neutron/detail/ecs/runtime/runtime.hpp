// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/runtime/run_env.hpp"
#include "neutron/detail/utility/macros.hpp"
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
concept run_payloay = requires(Payload& payload) {
    { payload.poll_events() } -> std::same_as<bool>;
    { payload.is_running() } -> std::same_as<bool>;
    payload.render_begin();
    payload.render_end();
};

template <
    scheduler_provider Sp, run_payloay Payload, typename Alloc, auto... Worlds>
class runtime {
public:
    template <typename Al = Alloc>
    constexpr runtime(Sp& sp, Payload* payload, const Al& alloc = {})
        : alloc_(alloc), payload_(payload), sp_(sp) {}

    constexpr int run() {
        using namespace ::neutron::execution;

        scheduler auto sch = sp_.get_scheduler();
        using run_envs_t   = run_envs_for<Alloc, Worlds...>;
        run_envs_t envs{ alloc_ };

        forward_progress_guarantee guarantee =
            get_forward_progress_guarantee(sch);

        if (guarantee == forward_progress_guarantee::weakly_parallel) {
            return _run_weak_parallel(envs, sch);
        }

        if constexpr (parallelism_scheduler_provider<Sp>) {
            if (sp_.available_parallelism() < std::tuple_size_v<run_envs_t>) {
                return _run_weak_parallel(envs, sch);
            }
        }

        return _run_parallel(envs, sch);
    }

private:
    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run_weak_parallel(Envs& envs, Scheduler scheduler) {
        using enum stage;
        _sync_run_stage<prestartup>(envs, scheduler);
        _sync_run_stage<startup>(envs, scheduler);
        _sync_run_stage<poststartup>(envs, scheduler);
        while (payload_->is_running()) {
            if (payload_->poll_events()) {
                continue;
            }
            _sync_run_stage<events>(envs, scheduler);
            _sync_run_stage<preupdate>(envs, scheduler);
            _sync_run_stage<update>(envs, scheduler);
            _sync_run_stage<postupdate>(envs, scheduler);
        }
        _sync_run_stage<last>(envs, scheduler);
        _sync_run_stage<shutdown>(envs, scheduler);

        return 0;
    }

    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run_parallel(Envs& envs, Scheduler scheduler) {
        using enum stage;
        _sync_run_stage<prestartup>(envs, scheduler);
        _sync_run_stage<startup>(envs, scheduler);
        _sync_run_stage<poststartup>(envs, scheduler);
        while (payload_->is_running()) {
            if (payload_->poll_events()) {
                continue;
            }
            _sync_run_stage<events>(envs, scheduler);
            _sync_run_stage<preupdate>(envs, scheduler);
            _sync_run_stage<update>(envs, scheduler);
            _sync_run_stage<postupdate>(envs, scheduler);
        }
        _sync_run_stage<last>(envs, scheduler);
        _sync_run_stage<shutdown>(envs, scheduler);

        return 0;
    }

    template <stage Stage, typename Envs, execution::scheduler Scheduler>
    void _sync_run_stage(Envs& envs, Scheduler scheduler) {
        using namespace execution;
        this_thread::sync_wait(envs.template run<Stage>(
            scheduler, std::chrono::system_clock::now()));
    }

    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    Payload* payload_;
    Sp& sp_; // NOLINT
};

template <
    auto... Worlds, scheduler_provider Sp, run_payloay Payload,
    typename Alloc = std::allocator<std::byte>>
ATOM_NODISCARD constexpr auto
    make_runtime(Sp& sp, Payload* payload, const Alloc& alloc = {}) {
    return runtime<Sp, Payload, Alloc, Worlds...>(sp, payload, alloc);
}

} // namespace neutron
