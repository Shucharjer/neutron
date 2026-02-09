#define ATOM_EXECUTION
#include <neutron/execution.hpp>
#include <neutron/print.hpp>
#include "require.hpp"

using namespace neutron;
using namespace neutron::execution;

int main() {
    {
        sender auto vsndr = just(32) | then([](auto val) { return val; });
        this_thread::sync_wait(vsndr);
    }

    {
        sender auto nsndr = just() | then([] { println("hello world"); });
        this_thread::sync_wait(nsndr);
    }

    {
        sender auto vsndr = just(42) | then([](auto val) {
                                println("first then: {}", val);
                                return val << 1;
                            }) |
                            then([](auto val) {
                                println("second then: {}", val);
                                return val << 1;
                            });
        auto opt = this_thread::sync_wait(vsndr);
        require(std::get<0>(opt.value()) == 42 << 2);
    }

    {
        sender auto nsndr = just() | then([] { println("first then"); }) |
                            then([] { println("second then"); });
        this_thread::sync_wait(nsndr);
    }

    return 0;
}
