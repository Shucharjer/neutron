#include <neutron/ecs.hpp>
#include "require.hpp"

using namespace neutron;
using enum stage;

namespace {

void test_world_without_systems_is_constructible() {
    constexpr auto desc = world_desc | add_systems<update>;

    auto world = make_world<desc>();
}

} // namespace

int main() {
    test_world_without_systems_is_constructible();
    return neutron_test::result();
}
