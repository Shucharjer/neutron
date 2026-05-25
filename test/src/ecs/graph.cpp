#include <cstddef>
#include <tuple>
#include <type_traits>
#include "neutron/detail/ecs/basic_commands.hpp"
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/task_graph.hpp"
#include "neutron/detail/ecs/world.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;

void foo() {}
void exec_plain() {}
void exec_plain_2() {}
void exec_plain_3() {}
void exec_plain_4() {}
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
    using desc1_first =
        type_list_element_t<0, typename desc1_stage::traits_list>;
    using desc1_second =
        type_list_element_t<1, typename desc1_stage::traits_list>;
    using desc1_graph = descriptor_graph<decltype(desc1)>;
    static_assert(
        std::same_as<
            typename desc1_graph::updates,
            staged_type_list<
                update, type_list<desc1_first>, type_list<desc1_second>>>);

    constexpr auto desc2 = add_systems<
        update, { exec_plain }, { exec_plain_2, after<exec_plain> }>(
        world_desc);
    using desc2_stage = stage_sysinfo<update, decltype(desc2)>;
    using desc2_first =
        type_list_element_t<0, typename desc2_stage::traits_list>;
    using desc2_second =
        type_list_element_t<1, typename desc2_stage::traits_list>;
    using desc2_graph = descriptor_graph<decltype(desc2)>;
    static_assert(
        std::same_as<
            typename desc2_graph::updates,
            staged_type_list<
                update, type_list<desc2_first>, type_list<desc2_second>>>);

    constexpr auto desc_multi_after =
        world_desc | add_systems<
                         update, exec_plain, exec_plain_2, exec_plain_3,
                         { exec_plain_4, after<exec_plain, exec_plain_2> }>;
    using desc_multi_after_graph =
        stage_graph<update, decltype(desc_multi_after)>;
    static_assert(
        std::same_as<
            typename desc_multi_after_graph::template successor_indices<0>,
            value_list<std::size_t{ 3 }>>);
    static_assert(
        std::same_as<
            typename desc_multi_after_graph::template successor_indices<1>,
            value_list<std::size_t{ 3 }>>);
    static_assert(
        std::same_as<
            typename desc_multi_after_graph::template predecessor_indices<3>,
            value_list<std::size_t{ 0 }, std::size_t{ 1 }>>);

    constexpr auto desc_multi_before =
        world_desc |
        add_systems<
            update, { exec_plain, before<exec_plain_2, exec_plain_3> },
            exec_plain_2, exec_plain_3, exec_plain_4>;
    using desc_multi_before_graph =
        stage_graph<update, decltype(desc_multi_before)>;
    static_assert(
        std::same_as<
            typename desc_multi_before_graph::template successor_indices<0>,
            value_list<std::size_t{ 1 }, std::size_t{ 2 }>>);
    static_assert(
        std::same_as<
            typename desc_multi_before_graph::template predecessor_indices<1>,
            value_list<std::size_t{ 0 }>>);
    static_assert(
        std::same_as<
            typename desc_multi_before_graph::template predecessor_indices<2>,
            value_list<std::size_t{ 0 }>>);

    using world0            = basic_world<decltype(desc_multi_before)>;
    using world0_tasks      = decltype(world0::template get_tasks<update>());
    using stage_task_graph0 = _stage_task_graph<world0_tasks>;

    static_assert(std::same_as<
                  typename world0_tasks::descriptor_type,
                  decltype(desc_multi_before)>);
    static_assert(world0_tasks::stage_value == update);
    static_assert(stage_task_graph0::size == 4);
    static_assert(stage_task_graph0::task_count == 4);
    static_assert(stage_task_graph0::predecessor_counts[0] == 0);
    static_assert(stage_task_graph0::predecessor_counts[1] == 1);
    static_assert(stage_task_graph0::predecessor_counts[2] == 1);
    static_assert(stage_task_graph0::successor_counts[0] == 2);
    static_assert(stage_task_graph0::successors[0][0] == 1);
    static_assert(stage_task_graph0::successors[0][1] == 2);
    static_assert(stage_task_graph0::local_task_indices[3] == 3);

    constexpr auto desc3 =
        world_desc | add_systems<
                         update, { &cmd_foo }, { &cmd_after, after<&cmd_foo> },
                         { &cmd_independent }>;
    using desc3_graph = stage_graph<update, decltype(desc3)>;
    static_assert(desc3_graph::command_node_count == 1);
    static_assert(desc3_graph::has_commands[0]);
    static_assert(!desc3_graph::has_commands[1]);
    static_assert(!desc3_graph::has_commands[2]);

    constexpr auto env_world0 =
        world_desc |
        add_systems<
            update, { exec_plain }, { exec_plain_2, after<exec_plain> }>;
    constexpr auto env_world1 =
        world_desc | add_systems<update, exec_plain_3, exec_plain_4>;
    using env_slot0 = basic_world<decltype(env_world0)>;
    using env_slot1 = basic_world<decltype(env_world1)>;
    using env_graph = _env_task_graph<
        decltype(env_slot0::template get_tasks<update>()),
        decltype(env_slot1::template get_tasks<update>())>;

    static_assert(env_graph::world_count == 2);
    static_assert(env_graph::task_count == 4);
    static_assert(env_graph::world_indices[0] == 0);
    static_assert(env_graph::world_indices[1] == 0);
    static_assert(env_graph::world_indices[2] == 1);
    static_assert(env_graph::world_indices[3] == 1);
    static_assert(env_graph::local_task_indices[0] == 0);
    static_assert(env_graph::local_task_indices[1] == 1);
    static_assert(env_graph::local_task_indices[2] == 0);
    static_assert(env_graph::local_task_indices[3] == 1);
    static_assert(env_graph::predecessor_counts[0] == 0);
    static_assert(env_graph::predecessor_counts[1] == 1);
    static_assert(env_graph::predecessor_counts[2] == 0);
    static_assert(env_graph::predecessor_counts[3] == 0);
    static_assert(env_graph::successor_counts[0] == 1);
    static_assert(env_graph::successors[0][0] == 1);
    static_assert(env_graph::max_concurrency == 3);

    return 0;
}
