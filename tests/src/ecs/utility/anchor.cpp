#include <cstddef>
#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <neutron/detail/ecs/utility/anchor.hpp>
#include "require.hpp"

using namespace neutron;

// data() returns a reference to the live pointer, so it follows reallocation.
static_assert(
    std::is_same_v<decltype(std::declval<anchor<int>&>().data()), int* const&>);
static_assert(std::is_same_v<
              decltype((std::declval<anchor<int>&>().data())), int* const&>);
static_assert(std::is_same_v<
              decltype(std::declval<const anchor<int>&>().data()),
              const int* const&>);
static_assert(std::is_same_v<
              decltype((std::declval<const anchor<int>&>().data())),
              const int* const&>);

namespace {

using int_anchor = anchor<int, std::allocator<int>>;

constexpr std::size_t reserved_capacity = 8;

void test_reserve_preserves_elements() {
    int_anchor values;
    values.emplace_back(3);
    values.push_back(5);

    values.reserve(reserved_capacity);

    require(values.size() == 2);
    require(values[0] == 3);
    require(values[1] == 5);
    require(values.capacity() >= reserved_capacity);
    require(*values.data() == 3);
    require(values.back() == 5);
}

void test_data_reference_follows_reallocation() {
    int_anchor values;
    values.emplace_back(3);
    values.push_back(5);
    auto* const& live_ptr       = values.data();
    const auto* before_relocate = live_ptr;

    values.reserve(reserved_capacity);

    require(live_ptr == values.data());
    require(live_ptr != before_relocate);

    const auto& const_values     = values;
    const auto* const& const_ptr = const_values.data();
    require(const_ptr == values.data());
}

void test_span_writes_through_to_storage() {
    int_anchor values;
    values.emplace_back(3);
    values.push_back(5);
    values.reserve(reserved_capacity);
    std::span<int> span(values);

    require(span.size() == 2);
    require(span[0] == 3);
    require(span[1] == 5);

    span[1] = 9;

    require(span[1] == 9);
    require(values[1] == 9);
    require(values.back() == 9);

    int sum = 0;
    for (int value : values) {
        sum += value;
    }
    require(sum == 3 + 9);
}

void test_moved_to_anchor_owns_the_elements() {
    int_anchor values;
    values.emplace_back(3);
    values.push_back(9);

    int_anchor moved(std::move(values));

    require(moved.size() == 2);
    require(moved[0] == 3);
    require(moved[1] == 9);

    moved.pop_back();

    require(moved.size() == 1);
    require(moved.back() == 3);

    moved.clear();

    require(moved.empty());
}

} // namespace

int main() {
    test_reserve_preserves_elements();
    test_data_reference_follows_reallocation();
    test_span_writes_through_to_storage();
    test_moved_to_anchor_owns_the_elements();
    return neutron_test::result();
}
