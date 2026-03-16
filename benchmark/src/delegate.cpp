#include <functional>
#include <benchmark/benchmark.h>
#include <neutron/functional.hpp>

using namespace neutron;

int add(int a, int b) { return a + b; }
struct Adder {
    int add(int a, int b) { return a + b; }
};
const auto lambda = [](int a, int b) { return a + b; };

static void BM_Delegate_FreeFunction(benchmark::State& state) {
    delegate<int(int, int)> delegate{ spread_arg<&add> };
    volatile int result = 0;
    for (auto _ : state) {
        result += delegate(1, 2);
    }
}
BENCHMARK(BM_Delegate_FreeFunction);
static void BM_StdFunction_FreeFunction(benchmark::State& state) {
    std::function<int(int, int)> func = &add;
    volatile int result               = 0;
    for (auto _ : state) {
        result += func(1, 2);
    }
}
BENCHMARK(BM_StdFunction_FreeFunction);

static void BM_Delegate_MemberFunction(benchmark::State& state) {
    Adder adder;
    delegate<int(int, int)> delegate{ spread_arg<&Adder::add>, adder };
    volatile int result = 0;
    for (auto _ : state) {
        result += delegate(1, 2);
    }
}
BENCHMARK(BM_Delegate_MemberFunction);
static void BM_StdFunction_MemberFunction(benchmark::State& state) {
    Adder adder;
    std::function<int(int, int)> func = [&adder](int a, int b) {
        return adder.add(a, b);
    };
    volatile int result = 0;
    for (auto _ : state) {
        result += func(1, 2);
    }
}
BENCHMARK(BM_StdFunction_MemberFunction);

static void BM_Delegate_Lambda(benchmark::State& state) {
    delegate<int(int, int)> delegate{ spread_arg<lambda> };
    volatile int result = 0;
    for (auto _ : state) {
        result += delegate(1, 2);
    }
}
BENCHMARK(BM_Delegate_MemberFunction);
static void BM_StdFunction_Lambda(benchmark::State& state) {
    std::function<int(int, int)> func = lambda;
    volatile int result               = 0;
    for (auto _ : state) {
        result += func(1, 2);
    }
}
BENCHMARK(BM_StdFunction_Lambda);

static void BM_Delegate_Construction(benchmark::State& state) {
    for (auto _ : state) {
        delegate<int(int, int)> del{ spread_arg<&add> };
        benchmark::DoNotOptimize(del);
    }
}
BENCHMARK(BM_Delegate_Construction);

static void BM_StdFunction_Construction(benchmark::State& state) {
    for (auto _ : state) {
        std::function<int(int, int)> func = &add;
        benchmark::DoNotOptimize(func);
    }
}
BENCHMARK(BM_StdFunction_Construction);

BENCHMARK_MAIN();
