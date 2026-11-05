#include <algorithm>
#include <vector>
#include <neutron/algorithm.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

void test_matches_std_lower_bound_for_every_position() {
    std::vector vec{ 1, 2, 8, 16 };

    constexpr int below_all     = 0;
    constexpr int first_element = 1;
    constexpr int between       = 4;
    constexpr int last_element  = 16;
    constexpr int above_all     = 17;

    require(
        ranges::branchless_lower_bound(vec, below_all) ==
        std::ranges::lower_bound(vec, below_all));
    require(
        ranges::branchless_lower_bound(vec, first_element) ==
        std::ranges::lower_bound(vec, first_element));
    require(
        ranges::branchless_lower_bound(vec, between) ==
        std::ranges::lower_bound(vec, between));
    require(
        ranges::branchless_lower_bound(vec, last_element) ==
        std::ranges::lower_bound(vec, last_element));
    require(
        ranges::branchless_lower_bound(vec, above_all) ==
        std::ranges::lower_bound(vec, above_all));
}

void test_empty_range_returns_end() {
    std::vector<int> vec;

    require(
        ranges::branchless_lower_bound(vec, 0) ==
        std::ranges::lower_bound(vec, 0));
}

} // namespace

int main() {
    test_matches_std_lower_bound_for_every_position();
    test_empty_range_returns_end();
    return neutron_test::result();
}
