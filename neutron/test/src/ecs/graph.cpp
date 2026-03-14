#include <array>
#include <type_traits>
#include "neutron/detail/ecs/basic_commands.hpp"
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/task_graph.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;

void foo() {}
void exec_plain() {}
void exec_plain_2() {}
void cmd_foo(basic_commands<>) {}
void cmd_after() {}
void cmd_independent() {}

int main() {
    constexpr auto desc0 =
        world_desc | add_systems<update, &foo> | execute<individual>;
    using desc0_stage = stage_sysinfo<update, decltype(desc0)>;
    using desc0_sys = type_list_element_t<0, typename desc0_stage::traits_list>;
    using desc0_graph = descriptor_graph<decltype(desc0)>;
    static_assert(std::same_as<
                  typename desc0_graph::updates,
                  staged_type_list<update, type_list<desc0_sys>>>);

    constexpr auto desc1 =
        world_desc | add_systems<update, &foo, { &foo, individual }>;
    using desc1_stage = stage_sysinfo<update, decltype(desc1)>;
    using desc1_first = type_list_element_t<0, typename desc1_stage::traits_list>;
    using desc1_second = type_list_element_t<1, typename desc1_stage::traits_list>;
    using desc1_graph = descriptor_graph<decltype(desc1)>;
    static_assert(std::same_as<
                  typename desc1_graph::updates,
                  staged_type_list<
                      update,
                      type_list<desc1_first>,
                      type_list<desc1_second>>>);

    constexpr auto desc2 = add_systems<
        update, { exec_plain }, { exec_plain_2, after<exec_plain> }>(world_desc);
    using desc2_stage = stage_sysinfo<update, decltype(desc2)>;
    using desc2_first = type_list_element_t<0, typename desc2_stage::traits_list>;
    using desc2_second = type_list_element_t<1, typename desc2_stage::traits_list>;
    using desc2_graph = descriptor_graph<decltype(desc2)>;
    static_assert(std::same_as<
                  typename desc2_graph::updates,
                  staged_type_list<
                      update,
                      type_list<desc2_first>,
                      type_list<desc2_second>>>);

    constexpr auto desc3 = world_desc |
                           add_systems<
                               update,
                               { &cmd_foo },
                               { &cmd_after, after<&cmd_foo> },
                               { &cmd_independent }>;
    using desc3_graph = stage_graph<update, decltype(desc3)>;
    static_assert(desc3_graph::command_node_count == 1);
    static_assert(desc3_graph::has_commands[0]);
    static_assert(!desc3_graph::has_commands[1]);
    static_assert(!desc3_graph::has_commands[2]);

    stage_task_graph<desc3_graph> runtime_graph;
    std::array<size_t, desc3_graph::size> ready{};

    auto ready_count = runtime_graph.collect_runnable(ready);
    if (ready_count != 2 || ready[0] != 0 || ready[1] != 2) {
        return 1;
    }

    runtime_graph.complete(0);
    if (!runtime_graph.dirty_commands() ||
        runtime_graph.state(1).pending_predecessors != 0 ||
        !runtime_graph.state(1).blocked_by_commands) {
        return 2;
    }

    ready_count = runtime_graph.collect_runnable(ready);
    if (ready_count != 1 || ready[0] != 2) {
        return 3;
    }

    runtime_graph.complete(2);
    if (!runtime_graph.needs_flush()) {
        return 4;
    }

    runtime_graph.flush();
    ready_count = runtime_graph.collect_runnable(ready);
    if (ready_count != 1 || ready[0] != 1) {
        return 5;
    }

    runtime_graph.complete(1);
    if (!runtime_graph.done()) {
        return 6;
    }

    return 0;
}
