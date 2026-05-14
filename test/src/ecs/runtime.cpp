#include "neutron/detail/ecs/runtime.hpp"
#include <neutron/ecs.hpp>
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

struct impl {
    bool poll_events() { return false; }
    bool is_stopped() { return false; }
    void render_begin() {}
    void render_end() {}
};

class app : impl {
    app() = default;

public:
    static app create() { return {}; }

    template <auto... Worlds>
    int run() {
        using namespace thread_pool_for_test;
        thread_pool pool;

        scheduler auto sch = pool.get_scheduler();
        auto rt = make_runtime<Worlds...>(sch, static_cast<impl*>(this));
        return rt.run();
    }
};

void foo() {}

constexpr auto world1 = world_desc | add_systems<update, foo>;
constexpr auto world2 =
    world_desc | add_systems<update, foo> | execute<group<2>>;
constexpr auto world3 =
    world_desc | add_systems<update, foo> | execute<individual>;
constexpr auto world4 =
    world_desc | add_systems<update, foo> | execute<group<2>>;
constexpr auto world5 =
    world_desc | add_systems<update, foo> | execute<individual>;

int main() {
    thread_pool pool;
    auto sch = pool.get_scheduler();
    auto rt  = make_runtime<world1, world2, world3, world4, world5>(
        sch, static_cast<impl*>(nullptr));

    // tasks<1-5>
    /*
    auto final_sender = a task group;
    cmdbufs bufs;
    schedule(sch) | then([&bufs, sch]{finish a task});
    sync_wait(when_all(...))
    */

    return app::create() | run_worlds<world1, world2, world3, world4, world5>();
}
