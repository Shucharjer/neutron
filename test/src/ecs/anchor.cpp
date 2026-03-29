#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <neutron/detail/ecs/anchor.hpp>

using namespace neutron;

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

auto test_anchor_exposes_vector_like_storage() -> int {
    anchor<int, std::allocator<int>> values;
    values.emplace_back(3);
    values.push_back(5);

    auto* const& live_ptr       = values.data();
    const auto* before_relocate = live_ptr;
    values.reserve(8);

    if (values.size() != 2) {
        return 1;
    }
    if (values[0] != 3 || values[1] != 5) {
        return 2;
    }
    if (values.capacity() < 8) {
        return 3;
    }
    if (*values.data() != 3) {
        return 6;
    }
    if (live_ptr != values.data() || live_ptr == before_relocate) {
        return 7;
    }
    if (values.back() != 5) {
        return 8;
    }

    std::span<int> span(values);
    if (span.size() != 2 || span[0] != 3 || span[1] != 5) {
        return 4;
    }
    span[1] = 9;
    if (span[1] != 9) {
        return 13;
    }
    if (values[1] != 9 || values.back() != 9) {
        return 14;
    }

    const auto& const_values     = values;
    const auto* const& const_ptr = const_values.data();
    if (const_ptr != values.data()) {
        return 9;
    }

    int sum = 0;
    for (int value : values) {
        sum += value;
    }
    if (sum != 12) {
        return 5;
    }

    anchor<int, std::allocator<int>> moved(std::move(values));
    if (moved.size() != 2 || moved[0] != 3 || moved[1] != 9) {
        return 10;
    }

    moved.pop_back();
    if (moved.size() != 1 || moved.back() != 3) {
        return 11;
    }

    moved.clear();
    return moved.empty() ? 0 : 12;
}

int main() { return test_anchor_exposes_vector_like_storage(); }
