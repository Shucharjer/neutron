#include <cstddef>
#include <neutron/execution.hpp>
#include <neutron/parallel.hpp>

using namespace neutron;
using namespace neutron::execution;

template <typename Alloc>
struct maker {
    auto operator()(const Alloc& alloc) { return 0; }
};

int main() {
    // static_context_thread_pool<maker> pool;
    // scheduler auto sch = pool.get_scheduler();
    // sender auto sndr   = schedule(sch);
    // sender auto sndr2  = then(sndr, [] {});
    // sync_wait(std::move(sndr));
    // run_loop loop;

    // sync_wait(loop.get_scheduler());

    return 0;
}
