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
    using allocator     = numa_allocator<std::byte>;
    using std_allocator = std::allocator<std::byte>;
    static_context_thread_pool<maker, std_allocator> pool;
    auto sch = pool.get_scheduler();

    return 0;
}
