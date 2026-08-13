// Benchmarks for neutron::small_vector vs std::vector
#include <vector>
#include <benchmark/benchmark.h>
#include <neutron/small_vector.hpp>

using namespace neutron;

static void BM_push_back_sbo(benchmark::State& st) {
    constexpr size_t SBO = 16;
    for (auto _ : st) {
        small_vector<int, SBO> v;
        for (size_t i = 0; i < SBO; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}

static void BM_push_back_heap(benchmark::State& st) {
    constexpr size_t SBO = 16;
    const size_t N       = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        small_vector<int, SBO> v;
        v.reserve(N);
        for (size_t i = 0; i < N; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}

static void BM_vector_push_back(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        std::vector<int> v;
        v.reserve(N);
        for (size_t i = 0; i < N; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}

static void BM_insert_middle(benchmark::State& st) {
    const size_t N = static_cast<size_t>(st.range(0));
    for (auto _ : st) {
        small_vector<int, 32> v;
        for (size_t i = 0; i < N; ++i) v.push_back(static_cast<int>(i));
        v.insert(v.begin() + static_cast<ptrdiff_t>(N / 2), 7);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_push_back_sbo)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_push_back_heap)->Arg(128)->Arg(1024)->Arg(1 << 14);
BENCHMARK(BM_vector_push_back)->Arg(128)->Arg(1024)->Arg(1 << 14);
BENCHMARK(BM_insert_middle)->Arg(128)->Arg(1024)->Arg(1 << 14);

BENCHMARK_MAIN();
