#include <neutron/ecs.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using enum stage;

template <>
constexpr bool neutron::as_component<int> = true;

void norm(query<with<int&>> query) {}

void echo(commands cmds, meta_accessor accessor) {
    for (auto typeinfo : accessor) {
        println("name: {}, hash: {}", typeinfo.name(), typeinfo.hash());
    }

    if (auto iter = accessor.find("int"); iter != accessor.end()) {
        println("found component: int");
    }
    if (auto iter = accessor.find("char"); iter == accessor.end()) {
        println("could not find component: char");
    }
}

int main() {
    constexpr auto desc = world_desc | add_systems<update, norm, echo>;
    auto world          = make_world<desc>();
    world.template call<update>();

    return 0;
}
