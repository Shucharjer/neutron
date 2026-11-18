#include <atomic>
#include <cstddef>
#include <memory>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include "require.hpp"
#include "thread_pool.hpp"

using namespace neutron;

// Not in the global namespace, where `shutdown` clashes with the winsock
// function declared by <windows.h>.
namespace {
using enum stage;
} // namespace

namespace {

using alloc_t = std::allocator<std::byte>;

struct Position {
    using component_concept = component_tag;
    float x{ 0 };
};

struct Velocity {
    using component_concept = component_tag;
    float x{ 0 };
};

int visited = 0;

void spawn_positions(commands cmds) {
    for (int i = 0; i < 4; ++i) {
        cmds.spawn(Position{ static_cast<float>(i) });
    }
}

void count_positions(query<with<Position&>> qry) {
    for (auto [position] : qry.get()) {
        static_cast<void>(position);
        ++visited;
    }
}

void test_query_parameter_sees_entities_spawned_earlier() {
    constexpr auto desc = world_desc | add_systems<startup, &spawn_positions> |
                          add_systems<update, &count_positions>;
    auto world     = make_world<desc>();
    auto scheduler = execution::inline_scheduler{};
    visited        = 0;

    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    this_thread::sync_wait(world.run<update>(scheduler, {}));

    require(visited == 4);
}

void move_positions(query<with<Position&>> qry) {
    for (auto [position] : qry.get()) {
        position.x += 10;
    }
}

float position_sum = 0;

void sum_positions(query<with<const Position&>> qry) {
    position_sum = 0;
    for (auto [position] : qry.get()) {
        position_sum += position.x;
    }
}

void test_writes_through_query_persist() {
    constexpr auto desc = world_desc | add_systems<startup, &spawn_positions> |
                          add_systems<update, &move_positions> |
                          add_systems<postupdate, &sum_positions>;
    auto world     = make_world<desc>();
    auto scheduler = execution::inline_scheduler{};

    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    this_thread::sync_wait(world.run<update>(scheduler, {}));
    this_thread::sync_wait(world.run<postupdate>(scheduler, {}));

    // 0 + 1 + 2 + 3, each moved by 10.
    require(position_sum == 46);
}

void spawn_moving(commands cmds) {
    cmds.spawn(Position{}, Velocity{});
    cmds.spawn(Position{}, Velocity{});
}

void test_archetypes_created_after_first_use_are_picked_up() {
    constexpr auto desc = world_desc | add_systems<startup, &spawn_positions> |
                          add_systems<preupdate, &spawn_moving> |
                          add_systems<update, &count_positions>;
    auto world     = make_world<desc>();
    auto scheduler = execution::inline_scheduler{};

    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    visited = 0;
    this_thread::sync_wait(world.run<update>(scheduler, {}));
    require(visited == 4);

    // preupdate creates the {Position, Velocity} archetype.
    this_thread::sync_wait(world.run<preupdate>(scheduler, {}));
    visited = 0;
    this_thread::sync_wait(world.run<update>(scheduler, {}));
    require(visited == 6);
}

void count_moving(query<with<const Position&, const Velocity&>> qry) {
    for (auto [position, velocity] : qry.get()) {
        static_cast<void>(position);
        static_cast<void>(velocity);
        ++visited;
    }
}

void test_filters_select_matching_archetypes_only() {
    constexpr auto desc = world_desc | add_systems<startup, &spawn_positions> |
                          add_systems<preupdate, &spawn_moving> |
                          add_systems<update, &count_moving>;
    auto world     = make_world<desc>();
    auto scheduler = execution::inline_scheduler{};

    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    this_thread::sync_wait(world.run<preupdate>(scheduler, {}));
    visited = 0;
    this_thread::sync_wait(world.run<update>(scheduler, {}));
    require(visited == 2);
}

std::atomic<int> parallel_visits = 0;

template <int>
void count_positions_in_parallel(query<with<const Position&>> qry) {
    for (auto [position] : qry.get()) {
        static_cast<void>(position);
        parallel_visits.fetch_add(1, std::memory_order_relaxed);
    }
}

void test_parallel_systems_share_one_cached_query() {
    constexpr auto desc =
        world_desc | add_systems<startup, &spawn_positions> |
        add_systems<
            update, system_spec{ &count_positions_in_parallel<0>, individual },
            system_spec{ &count_positions_in_parallel<1>, individual },
            system_spec{ &count_positions_in_parallel<2>, individual },
            system_spec{ &count_positions_in_parallel<3>, individual }>;
    run_envs_for<alloc_t, desc> envs;
    thread_pool_for_test::thread_pool pool;

    this_thread::sync_wait(envs.run<startup>(pool.get_scheduler(), {}));
    // The world changed in startup, so every system sees a stale cache and
    // they all race to synchronize it.
    parallel_visits = 0;
    this_thread::sync_wait(envs.run<update>(pool.get_scheduler(), {}));
    require(parallel_visits == 16);
}

void test_query_on_empty_world_yields_nothing() {
    constexpr auto desc = world_desc | add_systems<update, &count_positions>;
    auto world          = make_world<desc>();
    auto scheduler      = execution::inline_scheduler{};
    visited             = 0;

    this_thread::sync_wait(world.run<update>(scheduler, {}));

    require(visited == 0);
}

} // namespace

int main() {
    test_query_parameter_sees_entities_spawned_earlier();
    test_writes_through_query_persist();
    test_archetypes_created_after_first_use_are_picked_up();
    test_filters_select_matching_archetypes_only();
    test_parallel_systems_share_one_cached_query();
    test_query_on_empty_world_yields_nothing();
    return neutron_test::result();
}
