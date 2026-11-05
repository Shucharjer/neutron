#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

// align_up rounds up to the next multiple and keeps exact multiples.
static_assert(align_up(1U, 4) == 4);
static_assert(align_up(2U, 4) == 4);
static_assert(align_up(3U, 4) == 4);
static_assert(align_up(4U, 4) == 4);
static_assert(align_up(5U, 4) == 8);
static_assert(align_up(6U, 4) == 8);
static_assert(align_up(7U, 4) == 8);
static_assert(align_up(8U, 4) == 8);

// align_down rounds down to the previous multiple and keeps exact multiples.
static_assert(align_down(1U, 4) == 0);
static_assert(align_down(2U, 4) == 0);
static_assert(align_down(3U, 4) == 0);
static_assert(align_down(4U, 4) == 4);
static_assert(align_down(5U, 4) == 4);
static_assert(align_down(6U, 4) == 4);
static_assert(align_down(7U, 4) == 4);
static_assert(align_down(8U, 4) == 8);

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

} // namespace

int main() { return neutron_test::result(); }
