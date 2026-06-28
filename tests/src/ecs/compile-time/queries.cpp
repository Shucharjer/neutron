#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

template <>
constexpr bool ::neutron::as_component<int> = true;

void fn() {}
void qfn(query<with<int>> qry) {}

int main() {
    {
        constexpr auto desc = world_desc | add_systems<update, { fn }, { qfn }>;
        constexpr auto systems = get_systems<update>(desc);
    }

    return 0;
}
