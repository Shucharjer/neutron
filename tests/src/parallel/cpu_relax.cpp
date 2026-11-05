#include <atomic>
#include <thread>
#include <type_traits>
#include <neutron/parallel.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// cpu_relax is a plain hint with a positive spin budget.
static_assert(std::is_same_v<decltype(cpu_relax()), void>);
static_assert(max_spin_time > 0);

void test_relaxing_for_the_spin_budget_returns() {
    for (int i = 0; i < max_spin_time; ++i) {
        cpu_relax();
    }
}

// Passes by terminating: the loop must observe the store from the other thread.
void test_spin_wait_observes_other_thread() {
    std::atomic<bool> ready{ false };

    std::thread producer{ [&ready] {
        ready.store(true, std::memory_order_release);
    } };
    while (!ready.load(std::memory_order_acquire)) {
        cpu_relax();
    }
    producer.join();

    require(ready.load(std::memory_order_relaxed));
}

} // namespace

int main() {
    test_relaxing_for_the_spin_budget_returns();
    test_spin_wait_observes_other_thread();
    return neutron_test::result();
}
