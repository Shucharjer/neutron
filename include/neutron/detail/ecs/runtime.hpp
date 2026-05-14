#pragma once
#include <array>
#include <csignal>
#include <cstddef>
#include <new>
#include <tuple>
#include <utility>
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/world.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_interval.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_policy.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_buffer_count.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_min_concurrency.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/execution.hpp"

namespace neutron {

template <std::size_t GroupID, auto... Worlds>
class run_env_for_group;

template <auto World>
class run_env_for_individual;

template <typename, auto... Worlds>
struct _to_run_envs;
template <typename Curr, auto World>
struct _to_run_envs<Curr, World> {
    using policy = std::remove_const_t<decltype(get_execution_policy(World))>;
    using type   = decltype([] {
        if constexpr (std::same_as<policy, _individual_t>) {
            return type_list_cat_t<
                  Curr, type_list<run_env_for_individual<World>>>{};
        } else {
            return insert_tagged_value_list_inplace_t<policy, Curr, World>{};
        }
    }());
};
template <typename Curr, auto World, auto... Others>
struct _to_run_envs<Curr, World, Others...> {
    using policy = std::remove_const_t<decltype(get_execution_policy(World))>;
    using result = decltype([] {
        if constexpr (std::same_as<policy, _individual_t>) {
            return type_list_cat_t<
                Curr, type_list<run_env_for_individual<World>>>{};
        } else {
            return insert_tagged_value_list_inplace_t<policy, Curr, World>{};
        }
    }());
    using type   = typename _to_run_envs<result, Others...>::type;
};
template <auto... Worlds>
using _to_run_envs_t = typename _to_run_envs<type_list<>, Worlds...>::type;

template <typename>
struct _make_run_env_impl;
template <std::size_t GroupId, auto... Worlds>
struct _make_run_env_impl<tagged_value_list<_group_t<GroupId>, Worlds...>> {
    using type = run_env_for_group<GroupId, Worlds...>;
};
template <auto World>
struct _make_run_env_impl<run_env_for_individual<World>> {
    using type = run_env_for_individual<World>;
};

template <typename>
struct _make_run_envs;
template <template <typename...> typename Template, typename... Tys>
struct _make_run_envs<Template<Tys...>> {
    using type = Template<typename _make_run_env_impl<Tys>::type...>;
};

template <
    execution::scheduler Sch, typename RunEnvs, typename Payload,
    typename Alloc = std::allocator<std::byte>>
class runtime {
    using run_envs  = RunEnvs;
    using env_tuple = type_list_rebind_t<shared_tuple, run_envs>;

public:
    runtime(Sch& sch, Payload* payload) : sch_(sch), payload_(payload) {}

    runtime(Sch&& sch, Payload* payload)
        : sch_(std::move(sch)), payload_(payload) {}

    template <typename Al = Alloc>
    runtime(Sch& sch, Payload* payload, const Al& alloc)
        : sch_(sch), payload_(payload), alloc_(alloc) {}

    template <typename Al = Alloc>
    runtime(Sch&& sch, Payload* payload, const Al& alloc)
        : sch_(std::move(sch)), payload_(payload), alloc_(alloc) {}

    int run() {
        using namespace execution;
        using enum forward_progress_guarantee;

        const auto guarantee = get_forward_progress_guarantee(sch_);
        if (guarantee == forward_progress_guarantee::weakly_parallel) {
            _run_weak_parallel();
        } else {
            _run();
        }

        return 0;
    }

private:
    void _run_weak_parallel() {
        static bool run = true;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (get<Is>(envs_).startup(), ...);
            auto exit = [](int) noexcept { run = false; };
            signal(SIGINT, exit);
            signal(SIGTERM, exit);
            while (run) {
                if (payload_->poll_events()) {
                    if (payload_->is_stopped()) [[unlikely]] {
                        break;
                    }
                    continue;
                };
                // this_thread::sync_wait();
                (get<Is>(envs_).step(payload_), ...);
            }
            (get<Is>(envs_).shutdown(), ...);
        }(std::make_index_sequence<std::tuple_size_v<env_tuple>>());
    }

    void _run() {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (get<Is>(envs_).startup(), ...);
            // this_thread::sync_wait(
            //     (execution::schedule(sch_) | execution::then([&] {
            //          get<Is>(envs_).run(payload_);
            //      }))...);
            (get<Is>(envs_).shutdown(), ...);
        }(std::make_index_sequence<std::tuple_size_v<env_tuple>>());
    }

    Sch sch_;
    Payload* payload_;
    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    env_tuple envs_;
};

template <auto... Worlds, execution::scheduler Sch>
ATOM_NODISCARD auto make_runtime([[maybe_unused]] Sch&& sch, std::nullptr_t) {
    static_assert(false, "application payload is required");
}

template <auto... Worlds, execution::scheduler Sch, typename Payload>
ATOM_NODISCARD auto make_runtime(Sch&& sch, Payload* payload) {
    using run_envs = _make_run_envs<_to_run_envs_t<Worlds...>>::type;
    return runtime<std::decay_t<Sch>, run_envs, Payload>(
        std::forward<Sch>(sch), payload);
}

template <std::size_t GroupID, auto... Worlds>
class run_env_for_group {
public:
    static constexpr std::size_t world_count = sizeof...(Worlds);

    static constexpr std::size_t max_concurrency =
        (std::max)(get_max_concurrency(Worlds)...);

    static constexpr std::size_t total_max_concurrency =
        (get_max_concurrency(Worlds) + ...);

    void startup() {}

    template <typename Payload>
    void run(Payload* payload) {}

    template <typename Payload>
    void step(Payload* payload) {}

    void shutdown() {}

private:
    template <std::size_t Size>
    struct alignas(std::hardware_destructive_interference_size) _shared_buffer {
        std::array<command_buffer<>, Size> buf;
    };

    shared_tuple<std::pair<
        basic_world<decltype(Worlds)>,
        _shared_buffer<get_max_buffer_count(Worlds)>>...>
        worlds_;
};

template <auto World>
class alignas(std::hardware_destructive_interference_size)
    run_env_for_individual {
public:
    static constexpr std::size_t max_concurrency       = 1;
    static constexpr std::size_t total_max_concurrency = 1;

    run_env_for_individual()                                         = default;
    run_env_for_individual(const run_env_for_individual&)            = delete;
    run_env_for_individual(run_env_for_individual&&)                 = delete;
    run_env_for_individual& operator=(const run_env_for_individual&) = delete;
    run_env_for_individual& operator=(run_env_for_individual&&)      = delete;
    ~run_env_for_individual()                                        = default;

    void startup() {}

    template <typename Payload>
    void run(Payload* payload) {}

    template <typename Payload>
    void step(Payload* payload) {}

    void shutdown() {}

private:
    basic_world<decltype(World)> world_;
    command_buffer<> buf_;
};

} // namespace neutron
