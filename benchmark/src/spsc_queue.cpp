#include <cstdint>

#include <benchmark/benchmark.h>
#include <neutron/lockfree.hpp>

using namespace neutron;

using value_type                 = std::uint64_t;
using neutron_inplace_spsc_queue = inplace_spsc_queue<value_type, 32>;

template <typename SpscQueue>
void bm_spsc_queue_push_back(benchmark::State& state) {
    SpscQueue queue;
    value_type value = 0;

    queue.push_back(value++);

    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        queue.push_back(value++);
        benchmark::ClobberMemory();

        benchmark::DoNotOptimize(queue.front());
        queue.pop_front();
    }

    state.SetItemsProcessed(state.iterations());
}

template <typename SpscQueue>
void bm_spsc_queue_pop_front(benchmark::State& state) {
    SpscQueue queue;
    value_type value = 0;

    queue.push_back(value++);

    for (auto _ : state) {
        queue.push_back(value++);

        benchmark::DoNotOptimize(queue.front());
        queue.pop_front();
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(bm_spsc_queue_push_back, neutron_inplace_spsc_queue)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(bm_spsc_queue_pop_front, neutron_inplace_spsc_queue)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
