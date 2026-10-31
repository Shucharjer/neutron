#include <atomic>
#include <cstdint>
#include <thread>

#include <benchmark/benchmark.h>
#include <neutron/lockfree.hpp>

using namespace neutron;

using value_type                      = std::uint64_t;
using neutron_inplace_spsc_queue_32   = inplace_spsc_queue<value_type, 32>;
using neutron_inplace_spsc_queue_64   = inplace_spsc_queue<value_type, 64>;
using neutron_inplace_spsc_queue_128  = inplace_spsc_queue<value_type, 128>;
using neutron_inplace_spsc_queue_256  = inplace_spsc_queue<value_type, 256>;
using neutron_inplace_spsc_queue_512  = inplace_spsc_queue<value_type, 512>;
using neutron_inplace_spsc_queue_1024 = inplace_spsc_queue<value_type, 1024>;
using neutron_inplace_spsc_queue_2048 = inplace_spsc_queue<value_type, 2048>;
using neutron_inplace_spsc_queue_4096 = inplace_spsc_queue<value_type, 4096>;
using neutron_spsc_queue              = spsc_queue<value_type>;

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
            // A blocking push would never return once the consumer stops.
            if constexpr (requires { queue.try_push_back(value); }) {
                static_cast<void>(queue.try_push_back(value));
            } else {
                queue.push_back(value);
            }
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

#define BENCHMARK_QUEUE(queue_type)                                            \
    BENCHMARK_TEMPLATE(bm_spsc_queue_push_back, queue_type)                    \
        ->Unit(benchmark::kNanosecond);                                        \
    BENCHMARK_TEMPLATE(bm_spsc_queue_pop_front, queue_type)                    \
        ->Unit(benchmark::kNanosecond)

BENCHMARK_QUEUE(neutron_inplace_spsc_queue_32);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_64);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_128);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_256);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_512);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_1024);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_2048);
BENCHMARK_QUEUE(neutron_inplace_spsc_queue_4096);
BENCHMARK_QUEUE(neutron_spsc_queue);

BENCHMARK_MAIN();
