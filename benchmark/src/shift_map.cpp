#include <unordered_map>
#include <benchmark/benchmark.h>
#include <neutron/shift_map.hpp>

using namespace neutron;

void BM_unordered_map(benchmark::State& state) {
    std::unordered_map<uint64_t, void*> map;
    for (auto _ : state) {
        for (auto i = 0; i < state.range(); ++i) {
            map.emplace(i, nullptr);
        }
        map.clear();
    }
}

void BM_shift_map(benchmark::State& state) {
    shift_map<uint64_t, void*> map;
    for (auto _ : state) {
        for (auto i = 0; i < state.range(); ++i) {
            map.emplace(i, nullptr);
        }
        map.clear();
    }
}

BENCHMARK(BM_unordered_map)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_shift_map)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
