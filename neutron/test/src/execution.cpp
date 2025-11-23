#include <cstddef>
#include <memory>
#include <type_traits>
#include <neutron/execution.hpp>
#include <neutron/parallel.hpp>

using namespace neutron;
using namespace neutron::execution;

template <typename Alloc>
struct maker {
    auto operator()(const Alloc& alloc) { return 0; }
};

int main() {
    static_context_thread_pool<maker> pool;
    auto sch = pool.get_scheduler();
    schedule(sch);

    return 0;
}
