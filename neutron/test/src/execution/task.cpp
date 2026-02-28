#define ATOM_EXECUTION
#include <neutron/detail/execution/task.hpp>
#include <chrono>
#include <coroutine>
#include <thread>
#include <neutron/execution.hpp>
#include <neutron/execution_resources.hpp>


using namespace neutron;
using namespace neutron::execution;
using namespace std::chrono_literals;

static normthread thread;

class async_result {
public:
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_suspend(std::coroutine_handle<> handle) noexcept {
        handle_ = handle;
    }
    constexpr void await_resume() noexcept {
        if (handle_) {
        }
    }

private:
    std::coroutine_handle<> handle_;
};

int setresult() {
    std::this_thread::sleep_for(500ms);
    return 42;
}

task<int> getresult() {
    sender auto sndr =
        schedule(thread.get_scheduler()) | then([] { setresult(); });
    co_return co_await sndr;
}

int main() {
    auto opt       = this_thread::sync_wait(getresult());
    auto [integer] = opt.value();

    return 0;
}
