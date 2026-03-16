#include <neutron/ecs.hpp>
#include "app.hpp"

using namespace neutron;

using enum stage;

void foo() {}
void bar() {}
void nop() {}

int main() {
    // Render-stage systems may carry per-system ordering and execution tags.
    constexpr auto desc1 =
        world_desc | enable_render |
        add_systems<render, &foo, { &bar, after<&foo> }, { &nop, individual }>;

    // World-level execute metadata applies to the whole world.
    constexpr auto desc2 =
        world_desc | add_systems<update, &foo> | execute<frequency<1.0 / 60>>;

    // Local execute policies are still allowed under an individual world, as
    // long as they only describe per-system update behavior.
    constexpr auto desc3 =
        world_desc |
        add_systems<
            update, { &foo, frequency<1.0 / 30> }, { &nop, frequency<1.0> }> |
        execute<individual>;

    // Group and frequency can be combined at world scope.
    constexpr auto desc4 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, frequency<1.0 / 30>>;

    // Additional systems inherit the normalized world execute metadata.
    constexpr auto desc5 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, frequency<1.0 / 30>> |
                           add_systems<update, { &foo }>;

    // This file is a descriptor-syntax smoke test.
    mmapp::create() | run_worlds<desc1, desc2, desc3, desc4, desc5>();

    return 0;
}
