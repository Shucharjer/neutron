#include <neutron/ecs.hpp>
#include <neutron/print.hpp>
#include "require.hpp"

using namespace neutron;
using enum neutron::stage;

template <>
constexpr bool ::neutron::as_component<int> = true;

namespace {

constexpr int stored_value = 8;

bool first_call = true;

/// Stores a value on the first call; later calls report whether they see it.
int deref(local<int> local) {
    auto& [integer] = local;
    if (first_call) {
        first_call = false;
        integer    = stored_value;
        return 0;
    }
    return integer == stored_value ? 0 : 1;
}

void test_local_state_persists_across_constructions() {
    constexpr auto desc = world_desc | add_systems<update, deref>;
    auto world          = make_world<desc>();

    deref(construct_from_world<update, deref, local<int>>(world));
    const int mismatch =
        deref(construct_from_world<update, deref, local<int>>(world));

    require(mismatch == 0);
}

} // namespace

int main() {
    test_local_state_persists_across_constructions();
    return neutron_test::result();
}
