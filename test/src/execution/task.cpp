#define ATOM_EXECUTION
#include <chrono>
#include <coroutine>
#include <thread>
#include <neutron/detail/execution/coroutine_utilities/task.hpp>
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>

using namespace neutron;
using namespace neutron::execution;
using namespace std::chrono_literals;

static normthread thread;

// task<void> bar() {
//     co_await (just() | then([] {}));
//     co_return;
// }

// task<void> foo() {}

int main() {
    // auto opt       = this_thread::sync_wait(getresult());
    // auto [integer] = opt.value();

    return 0;
}
