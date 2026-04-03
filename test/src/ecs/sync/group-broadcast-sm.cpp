#include <chrono>
#include <string>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "app.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"

using namespace neutron;
using enum stage;
using enum strategy;

using time_point = decltype(std::chrono::high_resolution_clock::now());
struct msgpak {
    time_point tp;
    std::string msg;
};

void sender(sync<in_group, broadcast, master<msgpak>> sync) {
    auto& [master] = sync;
    auto& [mpm]    = master;
    mpm.broadcast(
        msgpak{ std::chrono::high_resolution_clock::now(), "the msg" });
}

void receiver(sync<in_group, broadcast, slave<msgpak>> sync) {
    auto& [slave] = sync;
    // auto& [mps] = slave;
    // while (mps) {
    //     auto msgpak = mps.recv();
    // }
}

constexpr auto sndr_world = world_desc | add_systems<pre_update, sender>;

constexpr auto rcvr_world = world_desc | add_systems<update, receiver>;

int main() {
    myapp::create() | run_worlds<sndr_world, rcvr_world, rcvr_world>();

    return 0;
}
