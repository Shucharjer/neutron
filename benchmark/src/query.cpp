#include <cstddef>
#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

struct Position {
    using component_concept = neutron::component_t;
    float x{ 0.0F };
    float y{ 0.0F };
};

struct Velocity {
    using component_concept = neutron::component_t;
    float x{ 0.0F };
    float y{ 0.0F };
};

using query_t           = query<with<Position&, const Velocity&>>;
using querior_t         = query_t::manual_querior_type;
using benchmark_slice_t = query_t::slice_t;
using benchmark_view_t  = query_t::view_t;

struct query_result {
    std::size_t matched = 0;
    float sum           = 0.0F;
};

static auto run_query_iteration(auto&& range) -> query_result {
    query_result result{};
    for (auto [position, velocity] : range) {
        ++result.matched;
        result.sum += position.x + velocity.y;
    }
    return result;
}

static auto run_slice_iteration(const benchmark_slice_t& slice)
    -> query_result {
    query_result result{};
    result.matched = slice.size();
    for (auto [position, velocity] : benchmark_view_t{ slice }) {
        result.sum += position.x + velocity.y;
    }
    return result;
}

static void publish(query_result result) {
    benchmark::DoNotOptimize(result.matched);
    benchmark::DoNotOptimize(result.sum);
    benchmark::ClobberMemory();
}

struct automatic_context {
    static void update(query_t query) {
        publish(run_query_iteration(query.get()));
    }
};

constexpr auto automatic_desc =
    world_desc | add_systems<update, &automatic_context::update>;

static void BM_query_manual_call(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range());

    basic_world<decltype(world_desc)> world;
    world.reserve(count);
    world.template reserve<Position, Velocity>(count);
    for (std::size_t i = 0; i < count; ++i) {
        world.spawn(
            Position{ static_cast<float>(i), static_cast<float>(i + 1U) },
            Velocity{
                static_cast<float>(i + 2U),
                static_cast<float>(i + 3U),
            });
    }

    // prefetch
    querior_t querior{ world };
    query_t query{ querior };
    publish(run_query_iteration(query.get()));

    for (auto _ : state) {
        querior_t querior{ world };
        query_t query{ querior };
        publish(run_query_iteration(query.get()));
    }
}

static void BM_query_automatic_call(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range());

    basic_world<decltype(automatic_desc)> world;
    world.reserve(count);
    world.template reserve<Position, Velocity>(count);
    for (std::size_t i = 0; i < count; ++i) {
        world.spawn(
            Position{ static_cast<float>(i), static_cast<float>(i + 1U) },
            Velocity{
                static_cast<float>(i + 2U),
                static_cast<float>(i + 3U),
            });
    }
    world.call<update>(); // update version, prefetch

    for (auto _ : state) {
        world.call<update>();
    }
}

static void BM_query_slice_raw(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range());

    archetype<> arche{ type_spreader<Position, Velocity>{} };
    arche.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        arche.emplace(
            static_cast<entity_t>(i + 1U),
            Position{ static_cast<float>(i), static_cast<float>(i + 1U) },
            Velocity{
                static_cast<float>(i + 2U),
                static_cast<float>(i + 3U),
            });
    }

    // prefetch
    const auto slice = slice_of<Position, Velocity>(arche);
    publish(run_slice_iteration(slice));

    for (auto _ : state) {
        const auto slice = slice_of<Position, Velocity>(arche);
        publish(run_slice_iteration(slice));
    }
}

BENCHMARK(BM_query_manual_call)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK(BM_query_automatic_call)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK(BM_query_slice_raw)
    ->Unit(benchmark::kMicrosecond)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 20);

BENCHMARK_MAIN();
