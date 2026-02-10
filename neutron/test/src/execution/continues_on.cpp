#define ATOM_EXECUTION
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    {
        affinity_thread at2(2);
        scheduler auto c2sch = at2.get_scheduler();
        affinity_thread at4(4);
        scheduler auto c4sch = at4.get_scheduler();

        auto job = then([](auto count) {
            int64_t num{};
            for (auto i = 0; i < count; ++i) {
                for (auto j = 0; j < count; ++j) {
                    num += (j & 0x1) == 0 ? j : -j;
                }
            }
            return count;
        });

        sender auto sndr = just(1024 * 256) | continues_on(c2sch) | job
            // | continues_on(c4sch) | job
            // | then([](auto result) { println("result: {}", result); })
            ;
        this_thread::sync_wait(sndr);
    }

    return 0;
}
