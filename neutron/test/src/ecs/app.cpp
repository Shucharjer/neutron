#include "app.hpp"
#include <neutron/concepts.hpp>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

void foo();

int main() {
    constexpr auto desc1 = world_desc | add_system<render, &foo>;

    constexpr auto desc2 = world_desc | add_system<update, &foo> |
                           set_schedule<frequency<1.0 / 60>>;

    constexpr auto desc3 = world_desc | add_system<update, &foo> |
                           set_schedule<individual, frequency<1.0 / 30>>;

    constexpr auto desc4 = world_desc | add_system<update, &foo> |
                           set_schedule<group<1>, frequency<1.0 / 30>>;

    constexpr auto desc5 = world_desc | add_system<update, &foo> |
                           set_schedule<group<1>, frequency<1.0 / 30>>;

    mmapp::create() | run_worlds<desc1, desc2, desc3, desc4, desc5>();

    return 0;
}
