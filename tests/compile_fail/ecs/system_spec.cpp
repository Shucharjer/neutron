#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

void first_task() {}
void second_task() {}
void third_task() {}
void missing_task() {}

#if NEUTRON_INVALID_SPEC == 1
constexpr auto desc =
    world_desc | add_systems<
                     update, { &first_task, after<&third_task> },
                     { &second_task, after<&first_task> },
                     { &third_task, after<&second_task> }>;
#elif NEUTRON_INVALID_SPEC == 2
constexpr auto desc =
    world_desc | add_systems<update, { &first_task, after<&missing_task> }>;
#elif NEUTRON_INVALID_SPEC == 3
constexpr auto desc =
    world_desc |
    add_systems<update, { &first_task, tick_rate<10>, tick_rate<20> }>;
#elif NEUTRON_INVALID_SPEC == 4
constexpr auto desc =
    world_desc | add_systems<update, { &first_task, group<1> }>;
#elif NEUTRON_INVALID_SPEC == 5
constexpr auto desc =
    world_desc | add_systems<update, { &first_task, individual, individual }>;
#else
    #error "define NEUTRON_INVALID_SPEC to a case from 1 through 5"
#endif

auto world = make_world<desc>();
