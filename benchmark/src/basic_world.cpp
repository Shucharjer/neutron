#include <vector>
#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>

using namespace neutron;

struct Position {
    using component_concept = neutron::component_t;
    float x{ 0 };
    float y{ 0 };
};

struct Velocity {
    using component_concept = neutron::component_t;
    float x{ 0 };
    float y{ 0 };
};

struct Tag {
    using component_concept = neutron::component_t;
};

static void BM_world_base_construct(benchmark::State& state) {
    for (auto _ : state) {
        world_base<> world;
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_base_construct_and_spawn(benchmark::State& state) {
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

static void BM_world_base_reserve_spawn(benchmark::State& state) {
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

static void BM_world_base_add_components_default(benchmark::State& state) {
    const auto count = static_cast<size_t>(state.range());
    for (auto _ : state) {
        world_base<> world;
        std::vector<entity_t> entities;

        state.PauseTiming();
        entities.reserve(count);
        world.template reserve<Position>(count);
        world.template reserve<Position, Velocity, Tag>(count);
        for (size_t i = 0; i < count; ++i) {
            entities.push_back(world.spawn(
                Position{ static_cast<float>(i), static_cast<float>(i + 1) }));
        }
        state.ResumeTiming();

        for (entity_t entity : entities) {
            world.add_components<Velocity, Tag>(entity);
        }

        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_base_add_components_value(benchmark::State& state) {
    const auto count = static_cast<size_t>(state.range());
    for (auto _ : state) {
        world_base<> world;
        std::vector<entity_t> entities;

        state.PauseTiming();
        entities.reserve(count);
        world.template reserve<Position>(count);
        world.template reserve<Position, Velocity>(count);
        for (size_t i = 0; i < count; ++i) {
            entities.push_back(world.spawn(
                Position{ static_cast<float>(i), static_cast<float>(i + 1) }));
        }
        state.ResumeTiming();

        for (size_t i = 0; i < count; ++i) {
            world.add_components(
                entities[i], Velocity{ static_cast<float>(2 * i),
                                       static_cast<float>(2 * i + 1) });
        }

        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_base_remove_components(benchmark::State& state) {
    const auto count = static_cast<size_t>(state.range());
    for (auto _ : state) {
        world_base<> world;
        std::vector<entity_t> entities;

        state.PauseTiming();
        entities.reserve(count);
        world.template reserve<Position, Velocity, Tag>(count);
        world.template reserve<Position>(count);
        for (size_t i = 0; i < count; ++i) {
            entities.push_back(world.spawn(
                Position{ static_cast<float>(i), static_cast<float>(i + 1) },
                Velocity{ static_cast<float>(2 * i),
                          static_cast<float>(2 * i + 1) },
                Tag{}));
        }
        state.ResumeTiming();

        for (entity_t entity : entities) {
            world.remove_components<Velocity, Tag>(entity);
        }

        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_world_base_construct)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_world_base_construct_and_spawn)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK(BM_world_base_reserve_spawn)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK(BM_world_base_add_components_default)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 18);

BENCHMARK(BM_world_base_add_components_value)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 18);

BENCHMARK(BM_world_base_remove_components)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 18);

BENCHMARK_MAIN();
