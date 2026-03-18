#include <type_traits>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/metainfo/render.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;

void foo() {}

// Marker type accepted through the nested `system_requirement_concept` alias.
struct direct_marker_requirement {
    using system_requirement_concept = system_requirement_t;
};

// Marker type accepted through `as_system_requirement` specialization.
struct adapted_requirement {};
struct invalid_requirement {};

namespace neutron {

template <>
constexpr bool as_system_requirement<adapted_requirement> = true;

} // namespace neutron

int main() {
    // Default world descriptors normalize to `group<0>` + `always`.
    using desc0_exec     = fetch_execinfo_t<decltype(world_desc)>;
    using desc0_policies = typename desc0_exec::value_list;
    static_assert(value_list_size_v<desc0_policies> == 2);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<0, desc0_policies>)>,
                  _group_t<0>>);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<1, desc0_policies>)>,
                  _always_t>);
    static_assert(execute_info<decltype(world_desc)>::is_grouped);
    static_assert(execute_info<decltype(world_desc)>::is_always);

    // World-level execute policies keep their normalized metadata intact.
    constexpr auto desc1 = world_desc | execute<frequency<1.0 / 30>>;
    using desc1_exec     = fetch_execinfo_t<decltype(desc1)>;
    using desc1_policies = typename desc1_exec::value_list;
    static_assert(value_list_size_v<desc1_policies> == 2);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<0, desc1_policies>)>,
                  _group_t<0>>);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<1, desc1_policies>)>,
                  _frequency_t<1.0 / 30>>);
    static_assert(execute_info<decltype(desc1)>::has_frequency);
    static_assert(execute_info<decltype(desc1)>::group_index == 0);
    static_assert(
        execute_info<decltype(desc1)>::frequency_interval == 1.0 / 30);
    static_assert(!execute_info<decltype(desc1)>::has_dynamic_frequency);

    constexpr auto desc2 =
        world_desc | execute<group<2>> | execute<dynamic_frequency<>>;
    using desc2_exec     = fetch_execinfo_t<decltype(desc2)>;
    using desc2_policies = typename desc2_exec::value_list;
    static_assert(value_list_size_v<desc2_policies> == 2);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<0, desc2_policies>)>,
                  _group_t<2>>);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<1, desc2_policies>)>,
                  _execute::template _dynamic_frequency_t<>>);
    static_assert(execute_info<decltype(desc2)>::group_index == 2);
    static_assert(execute_info<decltype(desc2)>::has_dynamic_frequency);

    constexpr auto desc3 =
        world_desc | add_systems<update, &foo> | execute<individual>;
    using desc3_exec     = fetch_execinfo_t<decltype(desc3)>;
    using desc3_policies = typename desc3_exec::value_list;
    static_assert(value_list_size_v<desc3_policies> == 2);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<0, desc3_policies>)>,
                  _individual_t>);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<1, desc3_policies>)>,
                  _always_t>);
    static_assert(execute_info<decltype(desc3)>::is_individual);
    static_assert(execute_info<decltype(desc3)>::is_always);

    constexpr auto desc4 = world_desc | execute<always>;
    using desc4_exec     = fetch_execinfo_t<decltype(desc4)>;
    using desc4_policies = typename desc4_exec::value_list;
    static_assert(value_list_size_v<desc4_policies> == 2);
    static_assert(std::same_as<
                  std::remove_cvref_t<
                      decltype(value_list_element_v<1, desc4_policies>)>,
                  _always_t>);

    // Event and render metadata are tracked independently from execute
    // policies.
    constexpr auto desc5 = world_desc | enable_render;
    using desc5_render   = fetch_renderinfo_t<decltype(desc5)>;
    using desc5_markers  = typename desc5_render::type_list;
    static_assert(type_list_size_v<desc5_markers> == 1);
    static_assert(render_info<decltype(desc5)>::is_enabled);
    static_assert(execute_info<decltype(desc5)>::is_grouped);
    static_assert(execute_info<decltype(desc5)>::is_always);

    constexpr auto desc6 = world_desc | enable_events;
    using desc6_events   = fetch_eventsinfo_t<decltype(desc6)>;
    using desc6_markers  = typename desc6_events::type_list;
    static_assert(type_list_size_v<desc6_markers> == 1);
    static_assert(events_info<decltype(desc6)>::is_enabled);
    static_assert(execute_info<decltype(desc6)>::is_grouped);
    static_assert(execute_info<decltype(desc6)>::is_always);

    // `sysdesc` accepts the standardized requirement markers.
    using namespace ::neutron::_add_system;

    static_assert(system_requirement<decltype(individual)>);
    static_assert(system_requirement<decltype(always)>);
    static_assert(system_requirement<decltype(group<1>)>);
    static_assert(system_requirement<decltype(frequency<1.0 / 30>)>);
    static_assert(system_requirement<decltype(before<&foo>)>);
    static_assert(system_requirement<decltype(after<&foo>)>);
    static_assert(system_requirement<direct_marker_requirement>);
    static_assert(system_requirement<adapted_requirement>);
    static_assert(!system_requirement<invalid_requirement>);
    static_assert(std::same_as<
                  decltype(sysdesc{ &foo, adapted_requirement{} }),
                  sysdesc<decltype(&foo), adapted_requirement{}>>);

    // Local execute policies inherit the execution domain and override only the
    // update policy.
    using merged_exec = merge_execinfo_t<
        fetch_execinfo_t<decltype(desc1)>,
        tagged_value_list<_execute::exec_tag_t, individual>>;
    static_assert(execinfo_traits<merged_exec>::is_individual);
    static_assert(execinfo_traits<merged_exec>::is_always);
    static_assert(!execinfo_traits<merged_exec>::has_frequency);

    // Invalid combinations are still rejected during validation.
    static_assert(!_metainfo::_validate_execinfo<tagged_value_list<
                      _execute::exec_tag_t, group<1>, individual>>::value);
    static_assert(!_metainfo::_validate_execinfo<tagged_value_list<
                      _execute::exec_tag_t, frequency<1.0 / 30>,
                      dynamic_frequency<>>>::value);
    static_assert(!_metainfo::_validate_local_execinfo<tagged_value_list<
                      _execute::exec_tag_t, dynamic_frequency<>>>::value);
    static_assert(!merge_execinfo<
                  tagged_value_list<_execute::exec_tag_t, individual>,
                  tagged_value_list<_execute::exec_tag_t, group<2>>>::value);

    return 0;
}
