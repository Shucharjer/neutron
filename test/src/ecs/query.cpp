#include <cstddef>
#include <memory>
#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/basic_querior.hpp"
#include "require.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

template <>
constexpr bool neutron::as_component<int> = true;

template <>
constexpr bool neutron::as_component<long> = true;

int observed_count = 0;
int observed_sum   = 0;

template <typename... Filters>
using querior = basic_querior<std::allocator<std::byte>, Filters...>;

void observe(query<with<int&>> q) {
    observed_count = 0;
    observed_sum   = 0;
    for (auto [value] : q.get()) {
        observed_sum += value;
        ++observed_count;
    }
}

auto test_manual_query_keeps_snapshot_behavior() -> int {
    basic_world<world_descriptor_t<>> world;
    const auto first = world.spawn(1);

    querior<with<int&>> q{ world };
    query<with<int&>> qry{ q };

    world.spawn(2, 3L);

    int sum   = 0;
    int count = 0;
    entity_t seen{};
    for (auto [entity, value] : qry.get_with_entity()) {
        seen = entity;
        sum += value;
        ++count;
    }

    require_or_return((count == 1), 1);
    require_or_return((sum == 1), 1);
    require_or_return((seen == first), 1);
    return 0;
}

auto test_cached_query_syncs_on_each_system_call() -> int {
    constexpr auto desc = world_desc | add_systems<update, &observe>;
    struct hooks {
        int polls = 0;

        bool poll_events() noexcept {
            ++polls;
            return true;
        }

        [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
    };
    hooks runtime_hooks;
    thread_pool pool(2);
    auto sch     = pool.get_scheduler();
    auto runtime = make_runtime<desc>(sch, &runtime_hooks);
    auto& world  = runtime.template run_env<0>().template get_world<0>();

    observed_count = 0;
    observed_sum   = 0;
    world.spawn(1);
    runtime.run();
    if (observed_count != 1 || observed_sum != 1) {
        return 2;
    }

    world.spawn(2, 3L);
    runtime_hooks.polls = 0;
    runtime.run();
    return observed_count == 2 && observed_sum == 3 ? 0 : 3;
}

int main() {
    if (const auto result = test_manual_query_keeps_snapshot_behavior();
        result != 0) {
        return result;
    }
    if (const auto result = test_cached_query_syncs_on_each_system_call();
        result != 0) {
        return result;
    }

    return 0;
}
