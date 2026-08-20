#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

using alloc_t = std::allocator<std::byte>;

template <auto Desc>
using world_of = basic_world<std::remove_cvref_t<decltype(Desc)>, alloc_t>;

void test_individual_env() {
    constexpr auto desc =
        world_desc | add_systems<update> | execute<individual>;

    // using run_envs = run_envs_for<alloc_t, desc>;
    // run_envs envs;
    // auto& [env] = envs;
}

int main() {
    test_individual_env();

    return 0;
}
