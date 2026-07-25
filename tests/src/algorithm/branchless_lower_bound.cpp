#include <algorithm>
#include <vector>
#include <neutron/algorithm.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    {
        std::vector vec{ 1, 2, 8, 16 };
        require_or_return(
            ranges::branchless_lower_bound(vec, 0) ==
                std::ranges::lower_bound(vec, 0),
            1);
        require_or_return(
            ranges::branchless_lower_bound(vec, 1) ==
                std::ranges::lower_bound(vec, 1),
            1);
        require_or_return(
            ranges::branchless_lower_bound(vec, 4) ==
                std::ranges::lower_bound(vec, 4),
            1);
        require_or_return(
            ranges::branchless_lower_bound(vec, 16) ==
                std::ranges::lower_bound(vec, 16),
            1);
        require_or_return(
            ranges::branchless_lower_bound(vec, 17) ==
                std::ranges::lower_bound(vec, 17),
            1);
    }

    {
        std::vector<int> vec;
        require_or_return(
            ranges::branchless_lower_bound(vec, 0) ==
                std::ranges::lower_bound(vec, 0),
            1);
    }

    return 0;
}
