#define ATOM_EXECUTION
#include <neutron/execution.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    sender auto vsndr = just(32);
    sender auto nsndr = just();

    auto vtuple = this_thread::sync_wait(vsndr).value();
    static_assert(std::same_as<decltype(vtuple), std::tuple<int>>);
    auto ntuple = this_thread::sync_wait(nsndr).value();
    static_assert(std::same_as<decltype(ntuple), std::tuple<>>);
    return 0;
}
