#include <neutron/ecs.hpp>
#include "app.hpp"

using namespace neutron;

using enum stage;

void foo() {}
void bar() {}
void nop() {}

int main() {
    constexpr auto desc1 =
        world_desc |
        add_systems<render, &foo, { &bar, after<&foo> }, { &nop, individual }>;

    constexpr auto desc2 =
        world_desc | add_systems<update, &foo> | execute<frequency<1.0 / 60>>;

    constexpr auto desc3 =
        world_desc |
        add_systems<
            update, { &foo, frequency<1.0 / 30> }, { &nop, frequency<1.0> }> |
        execute<individual>;

    constexpr auto desc4 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, frequency<1.0 / 30>>;

    constexpr auto desc5 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, frequency<1.0 / 30>> |
                           add_systems<update, { &foo }>;

    mmapp::create() | run_worlds<desc1, desc2, desc3, desc4, desc5>();

    return 0;
}
