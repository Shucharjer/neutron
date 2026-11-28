#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <neutron/ecs.hpp>
#endif
#include "require.hpp"

using namespace neutron;

// Not in the global namespace, where `shutdown` clashes with the winsock
// function declared by <windows.h>.
namespace {
using enum stage;
} // namespace

template <>
constexpr bool ::neutron::as_component<int> = true;

void fn() {}
void qfn(query<with<int>> qry) {}

int main() {
    // Systems with and without queries can share a stage.
    {
        constexpr auto desc = world_desc | add_systems<update, { fn }, { qfn }>;
        constexpr auto systems = get_systems<update>(desc);
    }

    return neutron_test::result();
}
