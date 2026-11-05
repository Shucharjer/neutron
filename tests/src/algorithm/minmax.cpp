#include <neutron/algorithm.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// Every permutation of { 0, 1, 2 }, so the extreme is tried in each position.

void test_min_returns_smallest_of_three() {
    require(min(0, 1, 2) == 0);
    require(min(0, 2, 1) == 0);
    require(min(1, 0, 2) == 0);
    require(min(1, 2, 0) == 0);
    require(min(2, 0, 1) == 0);
    require(min(2, 1, 0) == 0);
}

void test_max_returns_largest_of_three() {
    require(max(0, 1, 2) == 2);
    require(max(0, 2, 1) == 2);
    require(max(1, 0, 2) == 2);
    require(max(1, 2, 0) == 2);
    require(max(2, 0, 1) == 2);
    require(max(2, 1, 0) == 2);
}

} // namespace

int main() {
    test_min_returns_smallest_of_three();
    test_max_returns_largest_of_three();
    return neutron_test::result();
}
