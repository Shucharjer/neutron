#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// True if `padded<T>` is a `T` rounded up to whole cache lines.
template <typename T>
constexpr bool padded_to_line =
    sizeof(padded<T>) % hdi_size == 0 &&
    sizeof(padded<T>) == align_up(sizeof(T), hdi_size) &&
    alignof(padded<T>) >= alignof(T) && std::is_base_of_v<T, padded<T>> &&
    std::is_convertible_v<padded<T>*, T*>;

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

static_assert(std::has_single_bit(hdi_size));

// An empty type still occupies a whole line and keeps its members.
static_assert(sizeof(padded<empty>) == hdi_size);
static_assert(!std::is_empty_v<padded<empty>>);
static_assert(std::is_base_of_v<empty, padded<empty>>);
static_assert(padded<empty>::answer() == 42);

// Every type is rounded up to whole lines.
static_assert(padded_to_line<one_byte>);
static_assert(padded_to_line<with_tail>);
static_assert(padded_to_line<almost_line>);
static_assert(padded_to_line<over_line>);
static_assert(padded_to_line<counter>);
static_assert(padded_to_line<std::atomic<int>>);
static_assert(sizeof(padded<one_byte>) == hdi_size);
static_assert(sizeof(padded<over_line>) == 2 * hdi_size);

// Padding keeps the special members of the base.
static_assert(std::is_nothrow_default_constructible_v<padded<one_byte>>);
static_assert(std::is_trivially_copyable_v<padded<with_tail>>);

/// True if default member initializers survive and the base is writable.
constexpr bool default_member_initializers() {
    padded<with_tail> value;
    with_tail& base = value;

    base.second = 5;

    return value.first == 1 && value.second == 5;
}
static_assert(default_member_initializers());

void test_array_elements_are_one_line_apart() {
    constexpr std::size_t count = 4;
    padded<one_byte> values[count]; // NOLINT(modernize-avoid-c-arrays)

    for (std::size_t i = 0; i + 1 != count; ++i) {
        const auto lhs = reinterpret_cast<std::uintptr_t>(&values[i].value);
        const auto rhs = reinterpret_cast<std::uintptr_t>(&values[i + 1].value);
        require(rhs - lhs == hdi_size);
        require(values[i].value == 'x');
    }
}

void test_base_is_at_the_same_address_and_copied() {
    constexpr std::uint64_t written = 9;
    padded<with_tail> value;
    with_tail* const base = &value;
    require(static_cast<void*>(base) == static_cast<void*>(&value));

    base->first                  = written;
    const padded<with_tail> copy = value;

    require(copy.first == written);
    require(copy.second == 2);
}

void test_counters_on_separate_lines_count_concurrently() {
    constexpr std::size_t thread_count = 4;
    constexpr std::size_t rounds       = 10000;
    padded<counter> counters[thread_count]; // NOLINT(modernize-avoid-c-arrays)

    {
        std::jthread workers[thread_count]; // NOLINT(modernize-avoid-c-arrays)
        for (std::size_t i = 0; i != thread_count; ++i) {
            workers[i] = std::jthread([&counter = counters[i]] {
                for (std::size_t r = 0; r != rounds; ++r) {
                    counter.value.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
    } // Joins every worker.

    for (const auto& counter : counters) {
        require(counter.value.load(std::memory_order_relaxed) == rounds);
    }
}

} // namespace

int main() {
    test_array_elements_are_one_line_apart();
    test_base_is_at_the_same_address_and_copied();
    test_counters_on_separate_lines_count_concurrently();
    return neutron_test::result();
}
