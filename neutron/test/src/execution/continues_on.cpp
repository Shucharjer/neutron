#define ATOM_EXECUTION
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    {
        affinity_thread thread(2);
        scheduler auto sch = thread.get_scheduler();
        sender auto sndr =
            just(1024 * 1024) | continues_on(sch) | then([](auto count) {
                int64_t num{};
                for (auto i = 0; i < count; ++i) {
                    for (auto j = 0; j < count; ++j) {
                        num += (j & 0x1) == 0 ? j : -j;
                    }
                }
                return num;
            }) |
            then([](auto result) { println("result: {}", result); });
        this_thread::sync_wait(sndr);
    }

    return 0;
}
