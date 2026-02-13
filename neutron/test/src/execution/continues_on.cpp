#define ATOM_EXECUTION
#include <chrono>
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
            using namespace std::chrono;
            auto tp = high_resolution_clock::now();
            while (high_resolution_clock::now() - tp < 3s) {}
            return count;
        });

        auto run_on_c2 = continues_on(c2sch) | job;
        auto run_on_c4 = continues_on(c4sch) | job;
        auto output = then([](auto result) { println("result: {}", result); });
        sender auto sndr             = just(42) | run_on_c2;
        sender auto transfered       = sndr | run_on_c4;
        sender auto transfered_again = transfered | run_on_c2;

        this_thread::sync_wait(sndr | output);
        this_thread::sync_wait(transfered | output);
        this_thread ::sync_wait(transfered_again | output);
    }

    return 0;
}
