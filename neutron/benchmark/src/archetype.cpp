// Benchmarks for neutron::archetype common operations
#include <vector>
#include <benchmark/benchmark.h>
#include <neutron/ecs.hpp>

using namespace neutron;

struct Position {
    using component_concept = neutron::component_t;
    float x{ 0 }, y{ 0 };
};
struct Velocity {
    using component_concept = neutron::component_t;
    float vx{ 0 }, vy{ 0 };
};
struct TagEmpty {
    using component_concept = neutron::component_t;
};

static void BM_archetype_create(benchmark::State& st) {
    for (auto _ : st) {
        archetype<std::allocator<std::byte>> a{
            type_spreader<Position, Velocity, TagEmpty>{}
        };
        benchmark::DoNotOptimize(&a);
    }
}

static void BM_archetype_emplace_values(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        archetype<> a{ type_spreader<Position, Velocity>{} };
        a.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            a.emplace(
                static_cast<entity_t>(i + 1),
                Position{ float(i), float(i + 1) },
                Velocity{ float(2 * i), float(2 * i + 1) });
        }
        benchmark::DoNotOptimize(a.data());
        benchmark::ClobberMemory();
    }
}

static void BM_archetype_emplace_default(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        archetype<std::allocator<std::byte>> a{
            type_spreader<Position, Velocity, TagEmpty>{}
        };
        a.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            a.emplace(static_cast<entity_t>(i + 1));
        }
        benchmark::DoNotOptimize(a.data());
    }
}

static void BM_archetype_view_iter(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        archetype<std::allocator<std::byte>> a{
            type_spreader<Position, Velocity>{}
        };
        a.reserve(N);
        for (size_t i = 0; i < N; ++i)
            a.emplace(
                static_cast<entity_t>(i + 1),
                Position{ float(i), float(i + 1) },
                Velocity{ float(2 * i), float(2 * i + 1) });
        float acc = 0;
        for (auto [p, v] : a.view<Position&, Velocity&>()) {
            acc += p.x + p.y + v.vx + v.vy;
        }
        benchmark::DoNotOptimize(acc);
    }
}

static void BM_archetype_erase(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        archetype<std::allocator<std::byte>> a{
            type_spreader<Position, Velocity>{}
        };
        a.reserve(N);
        std::vector<entity_t> ids;
        ids.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            auto id = static_cast<entity_t>(i + 1);
            a.emplace(
                id, Position{ float(i), float(i + 1) },
                Velocity{ float(2 * i), float(2 * i + 1) });
            ids.push_back(id);
        }
        for (auto it = ids.rbegin(); it != ids.rend(); ++it)
            a.erase(*it);
        benchmark::DoNotOptimize(a.size());
    }
}

BENCHMARK(BM_archetype_create)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_archetype_emplace_values)->RangeMultiplier(2)->Range(64, 1 << 16);
BENCHMARK(BM_archetype_emplace_default)->RangeMultiplier(2)->Range(64, 1 << 16);
BENCHMARK(BM_archetype_view_iter)->RangeMultiplier(2)->Range(64, 1 << 16);
BENCHMARK(BM_archetype_erase)->RangeMultiplier(2)->Range(64, 1 << 16);

BENCHMARK_MAIN();
