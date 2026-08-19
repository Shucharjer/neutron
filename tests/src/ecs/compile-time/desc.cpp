#include <neutron/ecs.hpp>
#include "app.hpp"

using namespace neutron;

using enum stage;

void foo() {}
void bar() {}
void nop() {}

struct empty_application {
    static empty_application create() { return {}; }
    template <auto... Worlds>
    auto run() {}
};

int main() {
    // Render-stage systems may carry per-system ordering and execution tags.
    constexpr auto desc1 =
        world_desc | enable_render |
        add_systems<render, &foo, { &bar, after<&foo> }, { &nop, individual }>;

    // World-level execute metadata applies to the whole world.
    constexpr auto desc2 =
        world_desc | add_systems<update, &foo> | execute<tick_rate<60>>;

    // Local execute policies are still allowed under an individual world, as
    // long as they only describe per-system update behavior.
    constexpr auto desc3 =
        world_desc |
        add_systems<update, { &foo, tick_rate<30> }, { &nop, tick_rate<1> }> |
        execute<individual>;

    // Group and interval can be combined at world scope.
    constexpr auto desc4 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, tick_rate<30>>;
    static_assert(decltype(desc4)::_tick_rate == 30); // NOLINT

    // Additional systems inherit the normalized world execute metadata.
    constexpr auto desc5 = world_desc | add_systems<update, &foo> |
                           execute<group<1>, tick_rate<30>> |
                           add_systems<update, { &bar }>;

    constexpr auto desc6 = world_desc | set_identifier<"the sixth">;
    static_assert(decltype(desc6)::identifier == "the sixth");

    // This file is a descriptor-syntax smoke test.
    empty_application::create() |
        run_worlds<desc1, desc2, desc3, desc4, desc5, desc6>();

    return 0;
}
