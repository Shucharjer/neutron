#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

int main() {
    {
        constexpr auto desc = world_desc | add_systems<update>;
        auto world          = make_world<desc>();
    }

    return 0;
}
