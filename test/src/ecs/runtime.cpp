#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/runtime.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

class app {
    app() = default;

public:
    static app create() { return {}; }

    template <auto... Worlds>
    int run() {
        using namespace thread_pool_for_test;
        thread_pool pool;

        scheduler auto sch = pool.get_scheduler();
        auto rt            = make_runtime<Worlds...>(sch);
        return rt.run();
    }

private:
};

void foo() {}

constexpr auto world1 = world_desc | add_systems<update, foo>;

int main() {
    app::create() | run_worlds<world1>();
    return 0;
}
