#include <neutron/ecs.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using enum neutron::stage;

template <>
constexpr bool ::neutron::as_component<int> = true;

static bool once_flag = true;

int deref(local<int> local) {
    auto& [integer] = local;
    if (once_flag) {
        once_flag = false;
        integer   = 8;
        return 0;
    }
    return integer == 8 ? 0 : 1;
}

int main() {

    constexpr auto desc = world_desc | add_systems<update, deref>;

    auto world = make_world<desc>();

    deref(construct_from_world<update, deref, local<int>>(world));
    return deref(construct_from_world<update, deref, local<int>>(world));
}
