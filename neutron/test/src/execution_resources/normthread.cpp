#include <cstdint>
#include <iostream>
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    normthread thread;
    scheduler auto sch = thread.get_scheduler();
    auto sndr = just(1024ULL) | continue_on(sch) | then([](uint64_t val) {
                    uint64_t num{};
                    srand(time(nullptr));
                    for (auto i = 0; i < val; ++i) {
                        for (auto j = 0; j < val; ++j) {
                            if ((j & 0x1) == 0) {
                                num += rand();
                            } else {
                                num -= j;
                            }
                        }
                    }
                    return num;
                }) |
                then([](uint64_t num) { std::cout << num << '\n'; });

    sync_wait(std::move(sndr));

    return 0;
}
