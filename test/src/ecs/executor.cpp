#include <neutron/detail/ecs/executor.hpp>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

void fn1() {}
void fn2() {}

int main() {
    //
    {
        constexpr auto desc = world_desc | add_systems<update, fn1, fn2>;
        auto world          = make_world<desc>();
        using world_t       = decltype(world);
        using tasks_t       = decltype(world_t::get_tasks<update>());
        using task_graph_t  = tasks_t::graph_type;
        thread_pool pool;
        auto sch = pool.get_scheduler();
        execute_until_done<tasks_t>(sch);
    }

    return 0;
}
