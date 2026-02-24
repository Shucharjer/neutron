#define ATOM_EXECUTION
#include <atomic>
#include <cstdint>
#include <neutron/execution.hpp>
#include "require.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using namespace neutron::execution;

int main() {

    {
        constexpr auto count = 1024;
        std::atomic<uint32_t> num;
        sender auto sndr =
            just() | bulk(count, [&num](uint32_t size) { ++num; });
        this_thread::sync_wait(sndr);
        require_or_return(num == count, 1);
    }

    {
        constexpr auto count = 1024;
        std::atomic<uint32_t> num;
        thread_pool pool;
        scheduler auto sch = pool.get_scheduler();
        sender auto sndr =
            schedule(sch) | bulk(count, [&num](uint32_t size) { ++num; });
        this_thread::sync_wait(sndr);
        require_or_return(num == count, 1);
    }

    return 0;
}
