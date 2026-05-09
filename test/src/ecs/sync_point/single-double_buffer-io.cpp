#include <neutron/ecs.hpp>
#include "app.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;
using enum strategy;

void sender(::neutron::sync<single, double_buffer, output<int>> sync) {
    auto& [output] = sync;
    // auto& [iout]   = output;
    // iout.set(64);
}

void receiver(::neutron::sync<single, double_buffer, input<int>> sync) {
    auto& [input] = sync;
    // auto& [iin]   = input;
    // auto val = iin.use();
}

constexpr auto world = world_desc | add_systems<update, sender, receiver>;

int main() {
    myapp::create() | run_worlds<::world>();

    return 0;
}
