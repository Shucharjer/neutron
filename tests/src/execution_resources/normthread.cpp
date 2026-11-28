#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <cstdint>
    #include <cstdlib>
    #include <ctime>
    #include <iostream>
    #include <neutron/execution.hpp>
    #include <neutron/execution_resource.hpp>
#endif
#include "require.hpp"

using namespace neutron;
using namespace neutron::execution;

namespace {

void test_sender_completes_on_normthread() {
    normthread thread;
    scheduler auto sch = thread.get_scheduler();
    auto sndr = just(1024ULL) | continues_on(sch) | then([](std::uint64_t val) {
                    std::uint64_t num{};
                    std::srand(std::time(nullptr));
                    for (auto i = 0; i < val; ++i) {
                        for (auto j = 0; j < val; ++j) {
                            if ((j & 0x1) == 0) {
                                num += std::rand();
                            } else {
                                num -= j;
                            }
                        }
                    }
                    return num;
                }) |
                then([](std::uint64_t num) {
                    std::cout << num << '\n';
                });

    auto result = this_thread::sync_wait(std::move(sndr));

    require(result.has_value());
}

} // namespace

int main() {
    test_sender_completes_on_normthread();
    return neutron_test::result();
}
