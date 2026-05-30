#include <neutron/memory.hpp>

using namespace neutron;

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

int main() {

    static_assert(align_up(1U, 4) == 4);
    static_assert(align_up(2U, 4) == 4);
    static_assert(align_up(3U, 4) == 4);
    static_assert(align_up(4U, 4) == 4);
    static_assert(align_up(5U, 4) == 8);
    static_assert(align_up(6U, 4) == 8);
    static_assert(align_up(7U, 4) == 8);
    static_assert(align_up(8U, 4) == 8);

    static_assert(align_down(1U, 4) == 0);
    static_assert(align_down(2U, 4) == 0);
    static_assert(align_down(3U, 4) == 0);
    static_assert(align_down(4U, 4) == 4);
    static_assert(align_down(5U, 4) == 4);
    static_assert(align_down(6U, 4) == 4);
    static_assert(align_down(7U, 4) == 4);
    static_assert(align_down(8U, 4) == 8);

    return 0;
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
