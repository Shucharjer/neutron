#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <benchmark/benchmark.h>
#include <neutron/bytell_hash_map.hpp>
#include <neutron/flat_hash_map.hpp>
#include <neutron/shift_map.hpp>
#include <neutron/sparse_map.hpp>

// do NOT use 64 bits integer for sparse_map or shift_map!!!
using key_type                  = std::uint32_t;
using mapped_t                  = std::uint32_t;
using std_unordered_map_t       = std::unordered_map<key_type, mapped_t>;
using neutron_flat_hash_map_t   = neutron::flat_hash_map<key_type, mapped_t>;
using neutron_bytell_hash_map_t = neutron::bytell_hash_map<key_type, mapped_t>;
using neutron_sparse_map_t      = neutron::shift_map<key_type, mapped_t>;
using neutron_shift_map_t       = neutron::shift_map<key_type, mapped_t>;

[[nodiscard]] constexpr key_type mix_key(key_type value) noexcept {
    value ^= value >> 16;
    value *= 0x7FEB'352D; // NOLINT
    value ^= value >> 15; // NOLINT
    value *= 0x846C'A68B; // NOLINT
    value ^= value >> 16;
    return value;
}

template <typename HashMap>
[[nodiscard]] HashMap make_filled_map(std::size_t count) {
    HashMap map;
    map.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto key = mix_key(static_cast<key_type>(index + 1));
        map.emplace(key, key);
    }
    return map;
}

template <typename HashMap>
static void bm_hash_map_insert(benchmark::State& state) {
    HashMap map;
    map.reserve(static_cast<std::size_t>(state.max_iterations));
    key_t index = 1;
    for (auto _ : state) {
        const auto key = mix_key(index++);
        benchmark::DoNotOptimize(map.emplace(key, key));
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

template <typename HashMap>
static void bm_hash_map_find(benchmark::State& state) {
    constexpr std::size_t key_count = 1 << 16;
    const auto map                  = make_filled_map<HashMap>(key_count);
    std::size_t index               = 0;

    for (auto _ : state) {
        const auto key =
            mix_key(static_cast<key_type>((index++ & (key_count - 1)) + 1));
        auto iter = map.find(key);
        benchmark::DoNotOptimize(iter);
    }

    state.SetItemsProcessed(state.iterations());
}

template <typename HashMap>
static void bm_hash_map_erase(benchmark::State& state) {
    constexpr std::size_t key_count = 1 << 16;
    auto map                        = make_filled_map<HashMap>(key_count);
    std::size_t index               = 0;

    for (auto _ : state) {
        const auto key =
            mix_key(static_cast<key_type>((index++ & (key_count - 1)) + 1));
        benchmark::DoNotOptimize(map.erase(key));
        benchmark::ClobberMemory();

        state.PauseTiming();
        map.emplace(key, key);
        benchmark::ClobberMemory();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

#define NEUTRON_BENCHMARK_HASH_MAP_OPERATION(operation, map_type)              \
    BENCHMARK_TEMPLATE(operation, map_type)->Unit(benchmark::kNanosecond);

#define NEUTRON_BENCHMARK_HASH_MAP(map_type)                                   \
    NEUTRON_BENCHMARK_HASH_MAP_OPERATION(bm_hash_map_insert, map_type)         \
    NEUTRON_BENCHMARK_HASH_MAP_OPERATION(bm_hash_map_find, map_type)           \
    NEUTRON_BENCHMARK_HASH_MAP_OPERATION(bm_hash_map_erase, map_type)

NEUTRON_BENCHMARK_HASH_MAP(std_unordered_map_t)
NEUTRON_BENCHMARK_HASH_MAP(neutron_flat_hash_map_t)
NEUTRON_BENCHMARK_HASH_MAP(neutron_bytell_hash_map_t)
NEUTRON_BENCHMARK_HASH_MAP(neutron_sparse_map_t)
NEUTRON_BENCHMARK_HASH_MAP(neutron_shift_map_t)

#undef NEUTRON_BENCHMARK_HASH_MAP
#undef NEUTRON_BENCHMARK_HASH_MAP_OPERATION

BENCHMARK_MAIN();
