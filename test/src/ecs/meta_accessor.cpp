#include <neutron/ecs.hpp>
#include <neutron/print.hpp>
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

template <>
constexpr bool neutron::as_component<int> = true;

void norm(query<with<int&>> query) {}

void echo(commands cmds, meta_accessor accessor) {
    for (auto typeinfo : accessor) {
        println("name: {}, hash: {}", typeinfo.name(), typeinfo.hash());
    }

    if (const auto* iter = accessor.find("int"); iter != accessor.end()) {
        println("found component: int");
    }
    if (const auto* iter = accessor.find("char"); iter == accessor.end()) {
        println("could not find component: char");
    }
}

int main() {
    constexpr auto desc = world_desc | add_systems<update, norm, echo>;
    struct hooks {
        int polls = 0;

        bool poll_events() noexcept {
            ++polls;
            return true;
        }

        [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
    };

    thread_pool pool(2);
    auto sch = pool.get_scheduler();
    hooks runtime_hooks;
    auto runtime = make_runtime<desc>(sch, &runtime_hooks);
    runtime.run();

    return 0;
}
