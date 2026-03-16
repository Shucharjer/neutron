#define ATOM_EXECUTION
#include <cstdint>
#include <neutron/execution.hpp>
#include "require.hpp"

using namespace neutron;
using namespace neutron::execution;

int main() {
    // NOTE: finish when_all, then enable this.
    // {
    //     uint32_t num     = 0;
    //     auto inc         = [&num] { ++num; };
    //     sender auto sndr = when_all(just() | then(inc), just() | then(inc));
    //     this_thread::sync_wait(sndr);
    //     require_or_return(num == 1, 1);
    // }

    return 0;
}
