#include <chrono>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "app.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;
using enum strategy;

using time_point = decltype(std::chrono::high_resolution_clock::now());

void sender(sync<multi, broadcast, master<time_point>> sync) {
    //
}

void receiver(sync<multi, broadcast, slave<time_point>> sync) {
    //
}

constexpr auto sndr_world =
    world_desc | add_systems<update, sender> | execute<individual>;

constexpr auto rcvr_world = world_desc | add_systems<update, receiver>;

int main() {
    myapp::create() | run_worlds<sndr_world, rcvr_world, rcvr_world>();

    return 0;
}
