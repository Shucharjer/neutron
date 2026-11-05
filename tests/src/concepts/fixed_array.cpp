#include <array>
#include <cstddef>
#include <span>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Arbitrary class type used as an element.
struct foo {};

// fixed_array: satisfied by built-in arrays of known bound
static_assert(fixed_array<int[1]>);
static_assert(fixed_array<int[4]>);
static_assert(fixed_array<foo[2]>);
static_assert(fixed_array<const int[3]>);
static_assert(fixed_array<int[2][3]>);

// fixed_array: satisfied by std::array, including the empty one
static_assert(fixed_array<std::array<int, 3>>);
static_assert(fixed_array<std::array<int, 0>>);
static_assert(fixed_array<std::array<foo, 5>>);

// fixed_array: rejected
static_assert(!fixed_array<int>);
static_assert(!fixed_array<void>);
static_assert(!fixed_array<int*>);
static_assert(!fixed_array<int[]>);
static_assert(!fixed_array<int (*)[3]>);
static_assert(!fixed_array<std::vector<int>>);
static_assert(!fixed_array<std::span<int, 3>>);
static_assert(!fixed_array<foo>);

// fixed_array: rejected: references are not arrays
static_assert(!fixed_array<int (&)[3]>);
static_assert(!fixed_array<std::array<int, 3>&>);
static_assert(!fixed_array<std::array<int, 3>&&>);

// fixed_size: the outermost extent of a fixed array
static_assert(fixed_size<int[1]> == 1);
static_assert(fixed_size<int[4]> == 4);
static_assert(fixed_size<const int[3]> == 3);
static_assert(fixed_size<int[2][3]> == 2);
static_assert(fixed_size<std::array<int, 3>> == 3);
static_assert(fixed_size<std::array<int, 0>> == 0);
static_assert(fixed_size<std::array<foo, 5>> == 5);

// fixed_size: zero for anything else
static_assert(fixed_size<int> == 0);
static_assert(fixed_size<int[]> == 0);
static_assert(fixed_size<std::vector<int>> == 0);

static_assert(std::is_same_v<decltype(fixed_size<int[4]>), const std::size_t>);

} // namespace

int main() { return neutron_test::result(); }
