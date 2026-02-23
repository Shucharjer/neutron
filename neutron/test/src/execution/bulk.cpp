#define ATOM_EXECUTION
#include <atomic>
#include <cstdint>
#include <neutron/execution.hpp>
#include "require.hpp"

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

    return 0;
}
