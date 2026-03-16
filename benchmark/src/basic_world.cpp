#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>

using namespace neutron;

static void BM_world_base_spawn(benchmark::State& state) {
    for (auto _ : state) {
        world_base<> world;
        for (size_t i = 0; i < state.range(); ++i) {
            world.spawn();
        }
        world.clear();
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_base_reserve_swpan(benchmark::State& state) {
    world_base<> world;
    world.reserve(state.range());
    for (auto _ : state) {
        for (size_t i = 0; i < state.range(); ++i) {
            world.spawn();
        }
        world.clear();
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_world_base_spawn)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK(BM_world_base_reserve_swpan)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK_MAIN();
