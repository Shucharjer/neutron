#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "neutron/print.hpp"

using namespace neutron;
using enum stage;

using alloc_t = std::allocator<std::byte>;

void tick(global<std::uint32_t> glob) {
    auto& [count] = glob;
    ++count;
}

void println_ticks(global<std::uint32_t> glob) {
    auto& [count] = glob;
    println("ticks: {}", count);
}

constexpr auto main_world = world_desc | add_systems<preupdate, &tick> |
                            add_systems<shutdown, &println_ticks>;

void test_individual_env() {
    constexpr auto idvj_world = world_desc | add_systems<preupdate, &tick> |
                                add_systems<shutdown, &println_ticks> |
                                execute<individual, tick_rate<128>>;

    using run_envs = run_envs_for<alloc_t, main_world, idvj_world>;
}

int main() {
    test_individual_env();

    return 0;
}
