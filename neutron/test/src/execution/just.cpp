#define ATOM_EXECUTION
#include <neutron/execution.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    sender auto vsndr = just(32);
    sender auto nsndr = just();

    this_thread::sync_wait(vsndr);
    this_thread::sync_wait(nsndr);

    return 0;
}
