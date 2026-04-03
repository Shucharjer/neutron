#include <atomic>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "app.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;
using enum strategy;

void fn() {

}
void sys1() {

}
void sys2(sync<single, atomic_snap, inout<int>> sync) {
    auto& [sio] = sync;
    auto& [isio] = sio;
    isio.read(std::memory_order_relaxed);
}

constexpr auto world = world_desc|add_systems<update,sys1, sys2>;

int main() {
    myapp::create() | run_worlds<>();

    return 0;
}
