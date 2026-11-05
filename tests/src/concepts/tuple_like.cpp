#include <array>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// A user type opting in to the tuple protocol.
struct point {
    int x;
    int y;
};

/// A class that does not opt in.
struct plain {};

} // namespace

template <>
struct std::tuple_size<point> : std::integral_constant<std::size_t, 2> {};

template <std::size_t Index>
struct std::tuple_element<Index, point> {
    using type = int;
};

namespace {

// satisfied by
static_assert(tuple_like<std::tuple<int>>);
static_assert(tuple_like<std::tuple<int, float, char>>);
static_assert(tuple_like<std::pair<int, float>>);
static_assert(tuple_like<std::array<int, 3>>);
static_assert(tuple_like<point>);
static_assert(tuple_like<std::ranges::subrange<std::vector<int>::iterator>>);

// satisfied by: cv-qualified tuples
static_assert(tuple_like<const std::tuple<int>>);
static_assert(tuple_like<volatile std::pair<int, int>>);
static_assert(tuple_like<const std::array<int, 1>>);
static_assert(tuple_like<const point>);

// rejected: references are not tuple-like
static_assert(!tuple_like<std::tuple<int>&>);
static_assert(!tuple_like<const std::pair<int, int>&>);
static_assert(!tuple_like<std::array<int, 2>&&>);

// rejected: types without the tuple protocol
static_assert(!tuple_like<int>);
static_assert(!tuple_like<void>);
static_assert(!tuple_like<int*>);
static_assert(!tuple_like<int[3]>);
static_assert(!tuple_like<plain>);
static_assert(!tuple_like<std::vector<int>>);

} // namespace

int main() { return neutron_test::result(); }
