#include <cstddef>
#include <memory>
#include <thread>
#include <tuple>
#include <exec/static_thread_pool.hpp>
#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/world.hpp"

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

constexpr auto desc =
    world_desc | add_systems<
                     update, { &structural }, { &fn },
                     { &after_structural, after<&structural> }>;

constexpr auto grouped_desc =
    world_desc | add_systems<update, &grouped_world_system>;
constexpr auto individual_desc = world_desc | execute<individual> |
                                 add_systems<update, &individual_world_system>;

constexpr auto always_update_desc =
    world_desc | add_systems<update, &always_update_system>;
constexpr auto static_update_desc = world_desc | execute<frequency<1000.0>> |
                                    add_systems<update, &static_update_system>;
constexpr auto dynamic_update_desc =
    world_desc | execute<dynamic_frequency> |
    add_systems<update, &dynamic_update_system>;
constexpr auto group_one_desc =
    world_desc | execute<group<1>> | add_systems<update, &group_one_system>;
constexpr auto group_zero_desc =
    world_desc | execute<group<0>> | add_systems<update, &group_zero_system>;

int main() {
    auto world = make_world<desc>();
    call<update>(world);

    auto worlds = make_worlds<desc>();
    call<update>(worlds);

    exec::static_thread_pool pool(1);
    auto sch = pool.get_scheduler();
    std::vector<command_buffer<>> cmdbufs(1);

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

    auto grouped_worlds = make_worlds<group_one_desc, group_zero_desc>();
    call<update>(sch, cmdbufs, grouped_worlds);
    if (group_order_count != 2 || group_order[0] != 0 || group_order[1] != 1) {
        return 7;
    }

    return 0;
}
