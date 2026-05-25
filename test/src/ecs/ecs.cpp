#include <array>
#include <atomic>
#include <thread>
#include <tuple>
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
std::atomic<int> grouped_calls{ 0 };
std::atomic<int> individual_calls{ 0 };
int always_update_calls  = 0;
int static_update_calls  = 0;
int dynamic_update_calls = 0;
std::atomic<int> group_zero_calls{ 0 };
std::atomic<int> group_one_calls{ 0 };

void grouped_world_system() {
    grouped_thread_id = std::this_thread::get_id();
    grouped_calls.fetch_add(1, std::memory_order_acq_rel);
}

void individual_world_system() {
    individual_thread_id = std::this_thread::get_id();
    individual_calls.fetch_add(1, std::memory_order_acq_rel);
}

void always_update_system() { ++always_update_calls; }

void static_update_system() { ++static_update_calls; }

void dynamic_update_system() { ++dynamic_update_calls; }

void group_zero_system() {
    group_zero_calls.fetch_add(1, std::memory_order_acq_rel);
}

void group_one_system() {
    group_one_calls.fetch_add(1, std::memory_order_acq_rel);
}

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

// Update-interval variants.
constexpr auto always_update_desc =
    world_desc | add_systems<update, &always_update_system>;
constexpr auto static_update_desc = world_desc | execute<interval<1000.0>> |
                                    add_systems<update, &static_update_system>;
constexpr auto dynamic_update_desc =
    world_desc | execute<dynamic_interval<>> |
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
constexpr auto global_reader_desc =
    world_desc | add_systems<
                     update, { &increment_global },
                     { &observe_global, after<&increment_global> }>;

struct single_frame_hooks {
    int polls = 0;

    bool poll_events() noexcept {
        ++polls;
        return true;
    }

    [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
};

int main() {
    using namespace thread_pool_for_test;
    thread_pool pool(3);
    auto sch = pool.get_scheduler();

    // Runtime task execution preserves single-world dependency ordering.
    {
        no_command_order       = { -1, -1, -1 };
        no_command_order_count = 0;
        single_frame_hooks hooks;
        auto runtime = make_runtime<no_command_desc>(sch, &hooks);
        runtime.run();
        if (no_command_order_count != 3 || no_command_order[0] != 0 ||
            no_command_order[1] != 1 || no_command_order[2] != 2) {
            return 14;
        }
    }

    // Grouped and individual worlds are dispatched through different runtime
    // execution paths.
    {
        grouped_calls.store(0, std::memory_order_release);
        individual_calls.store(0, std::memory_order_release);

        single_frame_hooks hooks;
        auto runtime = make_runtime<grouped_desc, individual_desc>(sch, &hooks);
        runtime.run();

        if (grouped_calls.load(std::memory_order_acquire) != 1 ||
            individual_calls.load(std::memory_order_acquire) != 1) {
            return 1;
        }
        if (grouped_thread_id == std::thread::id{} ||
            individual_thread_id == std::thread::id{}) {
            return 2;
        }
    }

    // `always`, static interval and dynamic interval each keep their own
    // update semantics.
    {
        single_frame_hooks always_hooks;
        auto always_runtime =
            make_runtime<always_update_desc>(sch, &always_hooks);
        always_runtime.run();
        always_hooks.polls = 0;
        always_runtime.run();
        if (always_update_calls != 2) {
            return 3;
        }

        single_frame_hooks static_hooks;
        auto static_runtime =
            make_runtime<static_update_desc>(sch, &static_hooks);
        static_runtime.run();
        static_hooks.polls = 0;
        static_runtime.run();
        if (static_update_calls != 1) {
            return 4;
        }

        single_frame_hooks dynamic_hooks;
        auto dynamic_runtime =
            make_runtime<dynamic_update_desc>(sch, &dynamic_hooks);
        dynamic_runtime.run();
        auto& dynamic_world =
            dynamic_runtime.template run_env<0>().template get_world<0>();
        if (dynamic_update_calls != 0 ||
            dynamic_world.has_dynamic_update_interval()) {
            return 5;
        }
        dynamic_world.set_dynamic_update_interval(1000.0);
        dynamic_hooks.polls = 0;
        dynamic_runtime.run();
        dynamic_hooks.polls = 0;
        dynamic_runtime.run();
        if (dynamic_update_calls != 1) {
            return 6;
        }
    }

    // Groups are independent runtime environments. Their relative order is not
    // part of the contract; each group only needs to run exactly once here.
    {
        group_zero_calls.store(0, std::memory_order_release);
        group_one_calls.store(0, std::memory_order_release);

        single_frame_hooks hooks;
        auto runtime =
            make_runtime<group_one_desc, group_zero_desc>(sch, &hooks);
        runtime.run();
        if (group_zero_calls.load(std::memory_order_acquire) != 1 ||
            group_one_calls.load(std::memory_order_acquire) != 1) {
            return 7;
        }
    }

    // Runtime hooks require an explicit render world and still drive
    // the render callbacks around the frame.
    {
        grouped_calls.store(0, std::memory_order_release);
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
        test_hooks hooks;
        auto runner = make_runtime<render_runner_desc>(sch, &hooks);
        runner.run();
        if (grouped_calls.load(std::memory_order_acquire) != 1 ||
            hooks.render_begin_calls != 1 || hooks.render_end_calls != 1) {
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
            single_frame_hooks hooks;
            auto runtime = make_runtime<global_reader_desc>(sch, &hooks);
            runtime.run();
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
