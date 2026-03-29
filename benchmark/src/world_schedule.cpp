#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

void noop_system() {}
void noop_a() {}
void noop_b() {}
void noop_after() {}
void noop_cmd(basic_commands<>) {}
void noop_after_cmd() {}

constexpr auto empty_update_desc =
    world_desc | add_systems<update, &noop_system>;

constexpr auto layered_empty_update_desc =
    world_desc |
    add_systems<
        update, { &noop_a }, { &noop_b }, { &noop_after, after<&noop_a> }>;

constexpr auto command_update_desc =
    world_desc |
    add_systems<
        update, { &noop_cmd }, { &noop_after_cmd, after<&noop_cmd> }>;

static void BM_world_update_direct_empty_stage(benchmark::State& state) {
    auto world = make_world<empty_update_desc>();
    for (auto _ : state) {
        world.template call<update>();
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_call_update_empty_pipeline(benchmark::State& state) {
    auto world = make_world<empty_update_desc>();
    for (auto _ : state) {
        call_update(world);
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_update_direct_layered_empty_stage(
    benchmark::State& state) {
    auto world = make_world<layered_empty_update_desc>();
    for (auto _ : state) {
        world.template call<update>();
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

static void BM_world_update_direct_command_stage(benchmark::State& state) {
    auto world = make_world<command_update_desc>();
    for (auto _ : state) {
        world.template call<update>();
        benchmark::DoNotOptimize(world);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_world_update_direct_empty_stage)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_call_update_empty_pipeline)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_update_direct_layered_empty_stage)
    ->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_update_direct_command_stage)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
