#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <neutron/memory.hpp>

using namespace neutron;

namespace {

struct empty {
    ATOM_NODISCARD static constexpr int answer() noexcept { return 42; }
};

struct one_byte {
    char value = 'x';
};

struct with_tail {
    std::uint64_t first = 1;
    char second         = 2;
};

struct almost_line {
    std::byte bytes[hdi_size - 1]; // NOLINT(modernize-avoid-c-arrays)
};

struct over_line {
    std::byte bytes[hdi_size + 1]; // NOLINT(modernize-avoid-c-arrays)
};

struct counter {
    std::atomic<std::size_t> value{ 0 };
};

template <typename T>
constexpr bool padded_to_line =
    sizeof(padded<T>) % hdi_size == 0 &&
    sizeof(padded<T>) == align_up(sizeof(T), hdi_size) &&
    alignof(padded<T>) >= alignof(T) && std::is_base_of_v<T, padded<T>> &&
    std::is_convertible_v<padded<T>*, T*>;

static_assert(std::has_single_bit(hdi_size));

// An empty type still occupies a whole line.
static_assert(sizeof(padded<empty>) == hdi_size);
static_assert(!std::is_empty_v<padded<empty>>);
static_assert(std::is_base_of_v<empty, padded<empty>>);
static_assert(padded<empty>::answer() == 42);

static_assert(padded_to_line<one_byte>);
static_assert(padded_to_line<with_tail>);
static_assert(padded_to_line<almost_line>);
static_assert(padded_to_line<over_line>);
static_assert(padded_to_line<counter>);
static_assert(padded_to_line<std::atomic<int>>);
static_assert(sizeof(padded<one_byte>) == hdi_size);
static_assert(sizeof(padded<over_line>) == 2 * hdi_size);

static_assert(std::is_nothrow_default_constructible_v<padded<one_byte>>);
static_assert(std::is_trivially_copyable_v<padded<with_tail>>);

constexpr bool default_member_initializers() {
    padded<with_tail> value;
    with_tail& base = value;
    base.second     = 5;
    return value.first == 1 && value.second == 5;
}
static_assert(default_member_initializers());

bool test_array_separation() {
    padded<one_byte> values[4]; // NOLINT(modernize-avoid-c-arrays)
    for (std::size_t i = 0; i != 3; ++i) {
        const auto lhs = reinterpret_cast<std::uintptr_t>(&values[i].value);
        const auto rhs = reinterpret_cast<std::uintptr_t>(&values[i + 1].value);
        if (rhs - lhs != hdi_size || values[i].value != 'x') {
            return false;
        }
    }
    return true;
}

bool test_base_access() {
    padded<with_tail> value;
    with_tail* base = &value;
    if (static_cast<void*>(base) != static_cast<void*>(&value)) {
        return false;
    }
    base->first            = 9;
    padded<with_tail> copy = value;
    return copy.first == 9 && copy.second == 2;
}

bool test_concurrent_counters() {
    constexpr std::size_t threads = 4;
    constexpr std::size_t rounds  = 10000;
    padded<counter> counters[threads]; // NOLINT(modernize-avoid-c-arrays)
    {
        std::jthread workers[threads]; // NOLINT(modernize-avoid-c-arrays)
        for (std::size_t i = 0; i != threads; ++i) {
            workers[i] = std::jthread([&counter = counters[i]] {
                for (std::size_t r = 0; r != rounds; ++r) {
                    counter.value.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
    }
    for (const auto& counter : counters) {
        if (counter.value.load(std::memory_order_relaxed) != rounds) {
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    if (!test_array_separation()) {
        return 1;
    }
    if (!test_base_access()) {
        return 2;
    }
    if (!test_concurrent_counters()) {
        return 3;
    }
    return 0;
}
