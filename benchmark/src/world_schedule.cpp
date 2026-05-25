#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>
#include "thread_pool.hpp"

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
    add_systems<update, { &noop_cmd }, { &noop_after_cmd, after<&noop_cmd> }>;

struct benchmark_hooks {
    int polls = 0;

    bool poll_events() noexcept {
        ++polls;
        return true;
    }

    [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
};

template <auto Desc>
static void run_runtime_benchmark(benchmark::State& state) {
    thread_pool pool(2);
    auto sch = pool.get_scheduler();
    benchmark_hooks hooks;
    auto runtime = make_runtime<Desc>(sch, &hooks);
    for (auto _ : state) {
        hooks.polls = 0;
        runtime.run();
        benchmark::DoNotOptimize(runtime);
        benchmark::ClobberMemory();
    }
}

static void BM_world_update_direct_empty_stage(benchmark::State& state) {
    run_runtime_benchmark<empty_update_desc>(state);
}

static void BM_world_runtime_update_empty_pipeline(benchmark::State& state) {
    run_runtime_benchmark<empty_update_desc>(state);
}

static void
    BM_world_update_direct_layered_empty_stage(benchmark::State& state) {
    run_runtime_benchmark<layered_empty_update_desc>(state);
}

static void BM_world_update_direct_command_stage(benchmark::State& state) {
    run_runtime_benchmark<command_update_desc>(state);
}

BENCHMARK(BM_world_update_direct_empty_stage)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_runtime_update_empty_pipeline)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_update_direct_layered_empty_stage)
    ->Unit(benchmark::kNanosecond);
BENCHMARK(BM_world_update_direct_command_stage)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
