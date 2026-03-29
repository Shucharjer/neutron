#include "neutron/detail/ecs/metainfo.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/global.hpp"
#include "neutron/detail/ecs/world_descriptor/add_systems.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

using namespace neutron;
using namespace neutron::_metainfo;
using enum stage;

template <>
constexpr bool neutron::as_component<std::string> = true;

struct test_resource {
    int value = 0;
};

template <>
constexpr bool neutron::as_resource<test_resource> = true;

struct test_global {
    int value = 0;
};

void component_write(query<with<std::string&>>) {}
void component_read(query<with<const std::string&>>) {}
void component_copy(query<with<std::string>>) {}
void component_write_2(query<with<std::string&>>) {}

void resource_write(res<test_resource&>) {}
void resource_read(res<const test_resource&>) {}
void resource_write_2(res<test_resource&>) {}

void global_write(global<test_global&>) {}
void global_read(global<const test_global&>) {}
void global_write_2(global<test_global&>) {}

void mixed_component_and_resource(
    query<with<const std::string&>>, res<test_resource&>) {}
void mixed_global_and_component(
    global<test_global&>, query<with<std::string>>) {}

void exec_plain() {}
void exec_plain_2() {}

struct adapted_group_requirement {};

namespace neutron {

template <>
constexpr bool as_system_requirement<adapted_group_requirement> = true;

} // namespace neutron

template <typename Descriptor>
consteval bool validate_update_desc() {
    using validate_traits = stage_sysinfo<update, Descriptor>;
    using tagged_type     = typename validate_traits::tagged_type;
    using traits_list     = typename validate_traits::traits_list;
    using arg_lists       = typename validate_traits::arg_lists;
    using query_list      = typename validate_traits::query_list;
    using component_lists = typename validate_traits::component_lists;
    using resource_lists  = typename validate_traits::resource_lists;
    using global_lists    = typename validate_traits::global_lists;
    using access_keys     = typename validate_traits::access_keys;
    using write_keys      = typename validate_traits::write_keys;
    using world_execute   = typename validate_traits::world_execute;

    return validate_traits::value;
}

template <auto Descriptor>
inline constexpr bool validate_update_desc_v =
    validate_update_desc<decltype(Descriptor)>();

int main() {
    static_assert(validate_update_desc_v<world_desc>);

    constexpr auto read_only_desc =
        add_systems<update, component_read, component_copy>(world_desc);
    static_assert(validate_update_desc_v<read_only_desc>);

    constexpr auto component_rw_conflict =
        add_systems<update, { component_write }, { component_read }>(
            world_desc);
    static_assert(!validate_update_desc_v<component_rw_conflict>);

    constexpr auto component_ww_conflict =
        add_systems<update, { component_write }, { component_write_2 }>(
            world_desc);
    static_assert(!validate_update_desc_v<component_ww_conflict>);

    constexpr auto component_ordered = add_systems<
        update, { component_write }, { component_read, after<component_write> },
        { component_write_2, after<component_read> }>(world_desc);
    static_assert(validate_update_desc_v<component_ordered>);

    constexpr auto resource_conflict =
        add_systems<update, { resource_write }, { resource_read }>(world_desc);
    static_assert(!validate_update_desc_v<resource_conflict>);

    constexpr auto resource_ordered = add_systems<
        update, { resource_write }, { resource_read, after<resource_write> }>(
        world_desc);
    static_assert(validate_update_desc_v<resource_ordered>);

    constexpr auto global_conflict =
        add_systems<update, { global_write }, { global_read }>(world_desc);
    static_assert(!validate_update_desc_v<global_conflict>);

    constexpr auto global_ordered = add_systems<
        update, { global_write }, { global_read, after<global_write> }>(
        world_desc);
    static_assert(validate_update_desc_v<global_ordered>);

    constexpr auto mixed_desc = add_systems<
        update, mixed_component_and_resource, mixed_global_and_component>(
        world_desc);
    static_assert(validate_update_desc_v<mixed_desc>);

    constexpr auto world_group_desc =
        world_desc | execute<group<1>> | add_systems<update, exec_plain>;
    static_assert(validate_update_desc_v<world_group_desc>);
    using world_group_info = stage_sysinfo<update, decltype(world_group_desc)>;
    using world_group_sys =
        type_list_element_t<0, typename world_group_info::traits_list>;
    static_assert(
        std::same_as<typename world_group_sys::execution_policy, _group_t<1>>);

    constexpr auto system_group_override =
        world_desc | execute<group<1>> |
        add_systems<update, { exec_plain, adapted_group_requirement{} }>;
    static_assert(validate_update_desc_v<system_group_override>);
    using system_group_info =
        stage_sysinfo<update, decltype(system_group_override)>;
    using system_group_sys =
        type_list_element_t<0, typename system_group_info::traits_list>;
    static_assert(
        std::same_as<typename system_group_sys::execution_policy, _group_t<1>>);

    constexpr auto system_individual_override =
        world_desc | execute<group<1>, frequency<1.0 / 60>> |
        add_systems<update, { exec_plain, individual }>;
    static_assert(validate_update_desc_v<system_individual_override>);
    using system_individual_info =
        stage_sysinfo<update, decltype(system_individual_override)>;
    using system_individual_sys =
        type_list_element_t<0, typename system_individual_info::traits_list>;
    static_assert(
        std::same_as<
            typename system_individual_sys::execution_policy, _individual_t>);
    static_assert(system_individual_sys::execute_traits::is_always);
    static_assert(!system_individual_sys::execute_traits::has_frequency);

    constexpr auto cross_group_dependency =
        world_desc | execute<group<1>> |
        add_systems<
            update, { component_read },
            { component_write, group<2>, after<component_read> }>;
    static_assert(!validate_update_desc_v<cross_group_dependency>);

    constexpr auto cross_group_no_dependency =
        world_desc | execute<group<1>> |
        add_systems<update, { exec_plain }, { exec_plain_2, group<2> }>;
    static_assert(validate_update_desc_v<cross_group_no_dependency>);

    constexpr auto cyclic_dependency = add_systems<
        update, { exec_plain, after<exec_plain_2> },
        { exec_plain_2, after<exec_plain> }>(world_desc);
    static_assert(!validate_update_desc_v<cyclic_dependency>);

    return 0;
}
