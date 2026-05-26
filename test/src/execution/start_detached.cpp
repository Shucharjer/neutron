#include <atomic>
#include <chrono>
#include <thread>
#include <neutron/detail/execution/start_detached.hpp>
#include <neutron/execution.hpp>
#include "thread_pool.hpp"

using namespace neutron::execution;

int main() {
    thread_pool pool;
    auto sch = pool.get_scheduler();

    {
        std::atomic<bool> flag = true;
        auto sndr              = schedule(sch) | then([&flag] {
                        std::this_thread::sleep_for(std::chrono::seconds{ 1 });
                        flag = false;
                    });

        neutron::execution::start_detached(sndr);
        flag.wait(true);
    }

    {
        std::atomic<bool> flag = true;
        auto sndr              = schedule(sch) | then([&flag] {
                        std::this_thread::sleep_for(std::chrono::seconds{ 1 });
                        flag = false;
                    });

        neutron::execution::start_detached(std::move(sndr)); // NOLINT
        flag.wait(true);
    }

    return 0;
}
