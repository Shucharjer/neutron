// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/runtime/run_env.hpp"
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

template <stage Stage, typename Env>
static inline auto gather_tasks(Env& env) noexcept
// gather tasks should return a inplace_vector or sndr
{
    [&env]<std::size_t... Is>(std::index_sequence<Is...>) {
        (get<Is>(env), ...);
    }(std::make_index_sequence<std::tuple_size_v<Env>>());
}

template <
    scheduler_provider Sp, valid_payload Payload, typename Alloc,
    auto... Worlds>
class runtime {
public:
    template <typename Al = Alloc>
    constexpr runtime(Sp& sp, Payload* payload, const Al& alloc = {})
        : sp_(sp), alloc_(alloc) {}

    constexpr int run() {
        return 0;
        // using namespace ::neutron::execution;

        // scheduler auto sch = sp_.get_scheduler();
        // using run_envs_t   = run_envs_for<Alloc, Worlds...>;
        // run_envs_t envs{ alloc_ };

        // forward_progress_guarantee guarantee =
        //     get_forward_progress_guarantee(sch);

        // if (guarantee == forward_progress_guarantee::weakly_parallel) {
        //     return _run_weak_parallel(envs, sch);
        // }

        // if constexpr (parallelism_scheduler_provider<Sp>) {
        //     if (sp_.available_parallelism() < std::tuple_size_v<run_envs_t>) {
        //         return _run_weak_parallel(envs, sch);
        //     }
        // }

        // return _run_parallel(envs, sch);
    }

private:
    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run_weak_parallel(Envs& envs, Scheduler& scheduler) {
        using enum stage;
        [this, &envs,
         &scheduler]<std::size_t... Is>(std::index_sequence<Is...>) {
            // step until done
            (_step_stage<prestartup>(get<Is>(envs), scheduler), ...);
            (_step_stage<startup>(get<Is>(envs), scheduler), ...);
            (_step_stage<poststartup>(get<Is>(envs), scheduler), ...);
            while (true) {
                if (payload_->poll_events()) {
                    continue;
                }
                if (!payload_->is_running()) [[unlikely]] {
                    break;
                }
                (_step_stage<preupdate>(get<Is>(envs), scheduler), ...);
                (_step_stage<update>(get<Is>(envs), scheduler), ...);
                (_step_stage<postupdate>(get<Is>(envs), scheduler), ...);
            }
            (_step_stage<last>(get<Is>(envs), scheduler), ...);
            (_step_stage<shutdown>(get<Is>(envs), scheduler), ...);
        }(std::make_index_sequence<std::tuple_size_v<Envs>>());

        return 0;
    }

    template <typename Envs, execution::scheduler Scheduler>
    constexpr int _run_parallel(Envs& envs, Scheduler& scheduler) {
        using enum stage;
        [this, &envs,
         &scheduler]<std::size_t... Is>(std::index_sequence<Is...>) {
            (_run_stage<preupdate>(get<Is>(envs), scheduler), ...);
            (_run_stage<update>(get<Is>(envs), scheduler), ...);
            (_run_stage<postupdate>(get<Is>(envs), scheduler), ...);
            (_run_stage<first>(get<Is>(envs), scheduler), ...);
            (_run_stages(get<Is>(envs), scheduler), ...);
            (_run_stage<last>(get<Is>(envs), scheduler), ...);
            (_run_stage<shutdown>(get<Is>(envs), scheduler), ...);
        }(std::make_index_sequence<std::tuple_size_v<Envs>>());

        return 0;
    }

    template <stage Stage, typename Env, execution::scheduler Scheduler>
    void _step_stage(Env& env, Scheduler& scheduler) {
        [&env, scheduler]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto task = (get<Is>(env), ...);
        }(std::make_index_sequence<std::tuple_size_v<Env>>());
    }

    template <stage Stage, typename Env, execution::scheduler Scheduler>
    void _run_stage(Env& env, Scheduler& scheduler) {
        //
    }

    template <typename Env, execution::scheduler Scheduler>
    void _run_stages(Env& env, Scheduler& scheduler) {
        using namespace execution;
        auto fn = [&env, &scheduler] {
            [&env, scheduler]<std::size_t... Is>(std::index_sequence<Is...>) {
                while (true) {
                    // if constexpr (Env::enabled_events) {
                    //     //
                    // }
                    // sync_wait(schedule(scheduler) | then([] {}));
                    // if constexpr (Env::enabled_render) {
                    //     //
                    // }
                }
            }(std::make_index_sequence<std::tuple_size_v<Env>>());
        };
        sync_wait(schedule(scheduler) | then(fn));
    }

    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    Payload* payload_;
    Sp& sp_; // NOLINT
};

template <
    auto... Worlds, scheduler_provider Sp, valid_payload Payload,
    typename Alloc = std::allocator<std::byte>>
constexpr auto make_runtime(Sp& sp, Payload* payload, const Alloc& alloc = {}) {
    return runtime<Sp, Payload, Alloc, Worlds...>(sp, payload, alloc);
}

} // namespace neutron
