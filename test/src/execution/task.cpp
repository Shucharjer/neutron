#define ATOM_EXECUTION
#include <chrono>
#include <coroutine>
#include <thread>
#include <neutron/detail/execution/coroutine_utilities/task.hpp>
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>
#include "neutron/detail/execution/coroutine_utilities/task_scheduler.hpp"

using namespace neutron;
using namespace neutron::execution;
using namespace std::chrono_literals;

static normthread thread;

// TODO: update get_completion_scheduler
// task<void> bar() {
//     co_await just();
//     co_return;
// }

// task<void> foo() {}

int main() {
    // auto opt       = this_thread::sync_wait(getresult());
    // auto [integer] = opt.value();

    return 0;
}
