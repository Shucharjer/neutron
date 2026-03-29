#include <array>
#include <thread>
#include <tuple>
#include <vector>
#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/world.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

void fn() {}
void structural(basic_commands<>) {}
void after_structural() {}

std::thread::id grouped_thread_id{};
std::thread::id individual_thread_id{};
int grouped_calls        = 0;
int individual_calls     = 0;
int always_update_calls  = 0;
int static_update_calls  = 0;
int dynamic_update_calls = 0;
std::array<int, 2> group_order{ -1, -1 };
int group_order_count = 0;

void grouped_world_system() {
    grouped_thread_id = std::this_thread::get_id();
    ++grouped_calls;
}

void individual_world_system() {
    individual_thread_id = std::this_thread::get_id();
    ++individual_calls;
}

void always_update_system() { ++always_update_calls; }

void static_update_system() { ++static_update_calls; }

void dynamic_update_system() { ++dynamic_update_calls; }

void group_zero_system() { group_order[group_order_count++] = 0; }

void group_one_system() { group_order[group_order_count++] = 1; }

std::array<int, 3> no_command_order{ -1, -1, -1 };
int no_command_order_count = 0;

void no_command_first() { no_command_order[no_command_order_count++] = 0; }

void no_command_parallel() { no_command_order[no_command_order_count++] = 1; }

void no_command_after() { no_command_order[no_command_order_count++] = 2; }

struct shared_state {
    int value = 0;
};

void increment_global(global<shared_state&> state) {
    auto& [shared] = state;
    ++shared.value;
}

void observe_global(global<const shared_state&> state) {
    auto& [shared] = state;
    if (shared.value <= 0) {
        throw 1;
    }
}

// Dependency ordering inside a single update stage.
constexpr auto desc =
    world_desc | add_systems<
                     update, { &structural }, { &fn },
                     { &after_structural, after<&structural> }>;

// Worlds used to validate runtime execution behavior.
constexpr auto grouped_desc =
    world_desc | add_systems<update, &grouped_world_system>;
constexpr auto render_runner_desc =
    world_desc | enable_render | add_systems<update, &grouped_world_system>;
constexpr auto individual_desc = world_desc | execute<individual> |
                                 add_systems<update, &individual_world_system>;

// Update-frequency variants.
constexpr auto always_update_desc =
    world_desc | add_systems<update, &always_update_system>;
constexpr auto static_update_desc = world_desc | execute<frequency<1000.0>> |
                                    add_systems<update, &static_update_system>;
constexpr auto dynamic_update_desc =
    world_desc | execute<dynamic_frequency<>> |
    add_systems<update, &dynamic_update_system>;

// Grouped worlds used to validate scheduling order.
constexpr auto group_one_desc =
    world_desc | execute<group<1>> | add_systems<update, &group_one_system>;
constexpr auto group_zero_desc =
    world_desc | execute<group<0>> | add_systems<update, &group_zero_system>;

constexpr auto no_command_desc =
    world_desc | add_systems<
                     update, { &no_command_first }, { &no_command_parallel },
                     { &no_command_after, after<&no_command_first> }>;

// Global access used to validate cross-world shared state wiring.
constexpr auto global_writer_desc =
    world_desc | add_systems<update, &increment_global>;
constexpr auto global_reader_desc =
    world_desc | execute<group<1>> | add_systems<update, &observe_global>;

int main() {
    // Basic `call` overloads work for both a single world and a world tuple.
    {
        auto world = make_world<desc>();
        call<update>(world);

        auto worlds = make_worlds<desc>();
        call<update>(worlds);

        auto no_command_world = make_world<no_command_desc>();
        no_command_order      = { -1, -1, -1 };
        no_command_order_count = 0;
        call<update>(no_command_world);
        if (no_command_order_count != 3 || no_command_order[0] != 0 ||
            no_command_order[1] != 1 || no_command_order[2] != 2) {
            return 14;
        }
    }

    using namespace thread_pool_for_test;
    thread_pool pool(1);
    auto sch = pool.get_scheduler();
    std::vector<command_buffer<>> cmdbufs(1);

    // Grouped and individual worlds are dispatched through different runtime
    // execution paths.
    {
        auto scheduled_worlds = make_worlds<grouped_desc, individual_desc>();
        call<update>(sch, cmdbufs, scheduled_worlds);

        if (grouped_calls != 1 || individual_calls != 1) {
            return 1;
        }
        if (grouped_thread_id == std::thread::id{} ||
            individual_thread_id == std::thread::id{} ||
            grouped_thread_id == individual_thread_id) {
            return 2;
        }
    }

    // `always`, static frequency and dynamic frequency each keep their own
    // update semantics.
    {
        auto always_world = make_world<always_update_desc>();
        call_update(always_world);
        call_update(always_world);
        if (always_update_calls != 2) {
            return 3;
        }

        auto static_world = make_world<static_update_desc>();
        call_update(static_world);
        call_update(static_world);
        if (static_update_calls != 1) {
            return 4;
        }

        auto dynamic_world = make_world<dynamic_update_desc>();
        call_update(dynamic_world);
        if (dynamic_update_calls != 0 ||
            dynamic_world.has_dynamic_update_interval()) {
            return 5;
        }
        dynamic_world.set_dynamic_update_interval(1000.0);
        call_update(dynamic_world);
        call_update(dynamic_world);
        if (dynamic_update_calls != 1) {
            return 6;
        }
    }

    // Group ordering is deterministic, and the manual drive API can step each
    // group explicitly.
    {
        auto grouped_worlds = make_worlds<group_one_desc, group_zero_desc>();
        call<update>(sch, cmdbufs, grouped_worlds);
        if (group_order_count != 2 || group_order[0] != 0 ||
            group_order[1] != 1) {
            return 7;
        }

        group_order       = { -1, -1 };
        group_order_count = 0;
        auto grouped_schedule =
            make_world_schedule(sch, cmdbufs, grouped_worlds);
        int hook_count = 0;
        drive(
            grouped_schedule, hook([&] { ++hook_count; }), call_update_group<0>,
            hook([&](auto&) { ++hook_count; }), call_update_group<1>);
        if (hook_count != 2 || group_order_count != 2 || group_order[0] != 0 ||
            group_order[1] != 1) {
            return 8;
        }
    }

    // A schedule with hooks requires an explicit render world and still drives
    // the render callbacks around the frame.
    {
        grouped_calls      = 0;
        auto runner_worlds = make_worlds<render_runner_desc>();
        struct test_hooks {
            int polls              = 0;
            int render_begin_calls = 0;
            int render_end_calls   = 0;

            bool poll_events() {
                ++polls;
                return true;
            }

            bool is_stopped() const { return polls > 1; }

            void render_begin() { ++render_begin_calls; }
            void render_end() { ++render_end_calls; }
        };
        auto runner =
            make_world_schedule(test_hooks{}, sch, cmdbufs, runner_worlds);
        runner.run();
        if (grouped_calls != 1 || runner.hooks().render_begin_calls != 1 ||
            runner.hooks().render_end_calls != 1) {
            return 9;
        }
    }

    // Globals can be bound externally and then shared across worlds.
    {
        shared_state state{};
        if (has_global<shared_state>()) {
            return 10;
        }
        {
            scoped_global_binding<shared_state> binding{ state };
            if (!has_global<shared_state>() ||
                try_get_global<shared_state>() != &state) {
                return 11;
            }
            auto global_worlds =
                make_worlds<global_writer_desc, global_reader_desc>();
            call_update(sch, cmdbufs, global_worlds);
            if (state.value != 1 || &get_global<shared_state>() != &state) {
                return 12;
            }
        }
        if (has_global<shared_state>()) {
            return 13;
        }
    }

    return 0;
}
