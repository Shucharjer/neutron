#include <iostream>
#include <vector>
#include <exec/static_thread_pool.hpp>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

void fn() { std::cout << "Hello world\n"; }

constexpr auto desc = world_desc | add_system<update, &fn>;

using desc_traits = descriptor_traits<decltype(desc)>;
using world_t     = basic_world<std::remove_cvref_t<decltype(desc)>>;
using all         = world_t::run_lists::all;
using resources   = world_t::resources;
using locals      = world_t::locals;

int main() {
    auto world = make_world<desc>();
    exec::static_thread_pool pool;
    std::vector<command_buffer<>> cmdbufs(pool.available_parallelism());
    execution::scheduler auto sch = pool.get_scheduler();
    world.call<update>(sch, cmdbufs);

    return 0;
}
