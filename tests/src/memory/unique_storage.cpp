#include <memory>
#include <memory_resource>
#include <utility>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

using pmr_alloc_t = std::pmr::polymorphic_allocator<>;
using pmr_storage = unique_storage<int, pmr_alloc_t>;

// With std::allocator every operation is usable in constant evaluation.

// A default constructed storage holds nothing.
constexpr unique_storage<int> default_constructed;
static_assert(!default_constructed);

// Constructing from a value stores it.
constexpr int constructed_value = [] {
    unique_storage<int> storage{ 2 };
    return *storage;
}();
static_assert(constructed_value == 2);

// Move construction transfers the value, or the emptiness.
constexpr auto move_constructed_from_empty = [] {
    unique_storage<int> storage;
    auto moved = std::move(storage);
    return moved;
}();
static_assert(!static_cast<bool>(move_constructed_from_empty));

constexpr int move_constructed_value = [] {
    unique_storage<int> storage{ 2 };
    auto moved = std::move(storage);
    return *moved;
}();
static_assert(move_constructed_value == 2);

// Move assignment transfers the value; assigning from empty only needs to
// compile.
[[maybe_unused]] constexpr auto move_assigned_from_empty = [] {
    unique_storage<int> storage;
    unique_storage<int> moved;
    moved = std::move(storage);
    return moved;
}();

constexpr int move_assigned_value = [] {
    unique_storage<int> storage{ 1 };
    unique_storage<int> moved;
    moved = std::move(storage);
    return *moved;
}();
static_assert(move_assigned_value == 1);

// Swap exchanges the values; swapping with empty only needs to compile.
[[maybe_unused]] constexpr auto swapped_with_empty = [] {
    unique_storage<int> left{ 2 };
    unique_storage<int> right{};
    left.swap(right);
    return left;
}();

constexpr int swapped_value = [] {
    unique_storage<int> left{ 2 };
    unique_storage<int> right{ 4 };
    left.swap(right);
    return *left;
}();
static_assert(swapped_value == 4);

void test_pmr_default_constructed_is_empty() {
    const pmr_storage storage;

    require(!storage);
}

void test_pmr_immediately_allocates() {
    pmr_alloc_t alloc;

    const pmr_storage from_tag{ immediately };
    const pmr_storage from_tag_and_alloc{ immediately, alloc };
    const pmr_storage from_allocator_arg{ std::allocator_arg, alloc,
                                          immediately };

    require(from_tag);
    require(from_tag_and_alloc);
    require(from_allocator_arg);
}

void test_pmr_constructs_value_with_allocator() {
    pmr_alloc_t alloc;

    const pmr_storage storage{ std::allocator_arg, alloc, 2 };

    require(*storage == 2);
}

void test_pmr_move_construction_transfers_value() {
    pmr_alloc_t alloc;

    const auto from_empty = [&] {
        pmr_storage storage{ alloc };
        auto moved = std::move(storage);
        return moved;
    }();
    const int value = [&] {
        pmr_storage storage{ std::allocator_arg, alloc, 2 };
        auto moved = std::move(storage);
        return *moved;
    }();

    require(!static_cast<bool>(from_empty));
    require(value == 2);
}

void test_pmr_move_assignment_transfers_value() {
    // Assigning from empty is exercised but its result is not checked.
    [[maybe_unused]] const auto from_empty = [] {
        pmr_storage storage;
        pmr_storage moved;
        moved = std::move(storage);
        return moved;
    }();
    const int value = [] {
        pmr_storage storage{ 1 };
        pmr_storage moved;
        moved = std::move(storage);
        return *moved;
    }();

    require(value == 1);
}

void test_pmr_swap_exchanges_values() {
    // Swapping with empty is exercised but its result is not checked.
    [[maybe_unused]] const auto with_empty = [] {
        pmr_storage left{ 2 };
        pmr_storage right{};
        left.swap(right);
        return left;
    }();
    const int value = [] {
        pmr_storage left{ 2 };
        pmr_storage right{ 4 };
        left.swap(right);
        return *left;
    }();

    require(value == 4);
}

} // namespace

int main() {
    test_pmr_default_constructed_is_empty();
    test_pmr_immediately_allocates();
    test_pmr_constructs_value_with_allocator();
    test_pmr_move_construction_transfers_value();
    test_pmr_move_assignment_transfers_value();
    test_pmr_swap_exchanges_values();
    return neutron_test::result();
}
