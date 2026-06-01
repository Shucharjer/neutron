#include <cstddef>
#include <memory>
#include <neutron/detail/ecs/run_env.hpp>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

void test_individual_env() {
    constexpr auto desc =
        world_desc | add_systems<update> | execute<individual>;

    //
}

void test_env_for_group_with_single_world() {
    //
}

void test_env_for_group_with_multi_worlds() {
    //
}

int main() {
    test_individual_env();
    test_env_for_group_with_single_world();
    test_env_for_group_with_multi_worlds();

    return 0;
}
