#include <atomic>
#include <cstdint>
#include <thread>

#include <benchmark/benchmark.h>
#include <neutron/lockfree.hpp>

using namespace neutron;

using value_type                 = std::uint64_t;
using neutron_inplace_spsc_queue = inplace_spsc_queue<value_type, 32>;

template <typename SpscQueue>
void bm_spsc_queue_push_back(benchmark::State& state) {
    SpscQueue queue;

    std::atomic_bool run = true;
    std::jthread jt([&queue, &run] {
        while (run.load(std::memory_order_acquire)) {
            if (!queue.empty()) {
                queue.pop_front();
            }
        }
    });

    value_type val = 0;
    for (auto _ : state) {
        queue.push_back(val++);
    }

    run.store(false, std::memory_order_release);

    state.SetItemsProcessed(state.iterations());
}

template <typename SpscQueue>
void bm_spsc_queue_pop_front(benchmark::State& state) {
    SpscQueue queue;

    std::atomic_bool run = true;
    std::jthread jt([&queue, &run] {
        value_type value = 0;
        while (run.load(std::memory_order_acquire)) {
            queue.push_back(value);
        }
    });

    for (auto _ : state) {
        if (!queue.empty()) {
            queue.pop_front();
        }
    }

    run.store(false, std::memory_order_release);
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(bm_spsc_queue_push_back, neutron_inplace_spsc_queue)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(bm_spsc_queue_pop_front, neutron_inplace_spsc_queue)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
