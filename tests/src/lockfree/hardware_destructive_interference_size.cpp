#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <neutron/lockfree.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Two atomics placed on separate cache lines.
struct split {
    alignas(hdi_size) std::atomic<int> lhs;
    alignas(hdi_size) std::atomic<int> rhs;
};

// hdi_size is a constant power of two, at least the fundamental alignment.
static_assert(std::is_same_v<decltype(hdi_size), const std::size_t>);
static_assert(hdi_size != 0);
static_assert(std::has_single_bit(hdi_size));
static_assert(hdi_size >= alignof(std::max_align_t));

// Usable as an alignment to keep hot fields on separate cache lines.
static_assert(alignof(split) == hdi_size);
static_assert(offsetof(split, rhs) - offsetof(split, lhs) >= hdi_size);

void test_split_fields_are_line_aligned() {
    const split value{};

    const auto lhs = reinterpret_cast<std::uintptr_t>(&value.lhs);
    const auto rhs = reinterpret_cast<std::uintptr_t>(&value.rhs);

    require(lhs % hdi_size == 0);
    require(rhs % hdi_size == 0);
}

void test_split_fields_hold_independent_values() {
    split value{};

    value.lhs.store(1, std::memory_order_relaxed);
    value.rhs.store(2, std::memory_order_relaxed);

    require(value.lhs.load() == 1);
    require(value.rhs.load() == 2);
}

} // namespace

int main() {
    test_split_fields_are_line_aligned();
    test_split_fields_hold_independent_values();
    return neutron_test::result();
}
