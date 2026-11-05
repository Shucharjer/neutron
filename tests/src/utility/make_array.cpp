#include <array>
#include <cstddef>
#include <numeric>
#include <string_view>
#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

namespace {

/// Constructible from int only explicitly.
struct explicit_only {
    int value;
    constexpr explicit explicit_only(int val) noexcept : value(val) {}
};

/// Converting constructor that derives a second member.
struct point {
    int x;
    int y;
    constexpr point(int val) noexcept : x(val), y(val * 2) {}
};

// The size is the number of arguments.
constexpr auto ints = neutron::make_array<int>(1, 2, 3);
static_assert(std::is_same_v<decltype(ints), const std::array<int, 3>>);
static_assert(ints[0] == 1);
static_assert(ints[1] == 2);
static_assert(ints[2] == 3);

// An empty argument list yields an empty array.
constexpr auto none = neutron::make_array<int>();
static_assert(std::is_same_v<decltype(none), const std::array<int, 0>>);

// Each element is brace-initialized from one argument.
constexpr auto widened = neutron::make_array<long long>(1, 2L, 'c');
static_assert(
    std::is_same_v<decltype(widened), const std::array<long long, 3>>);
static_assert(widened[2] == 'c');

constexpr auto doubles = neutron::make_array<double>(1.5F, 2.5);
static_assert(doubles[0] == 1.5);
static_assert(doubles[1] == 2.5);

constexpr auto views = neutron::make_array<std::string_view>("a", "bc", "def");
static_assert(views[0] == "a");
static_assert(views[1] == "bc");
static_assert(views[2].size() == 3);

// Explicit constructors are allowed by direct-list-initialization.
constexpr auto explicits = neutron::make_array<explicit_only>(4, 5);
static_assert(explicits[0].value == 4);
static_assert(explicits[1].value == 5);

constexpr auto points = neutron::make_array<point>(1, 3);
static_assert(points[1].x == 3);
static_assert(points[1].y == 6);

// The helper is also exported into the global namespace.
constexpr auto global = ::make_array<char>('x', 'y');
static_assert(global[0] == 'x');
static_assert(global.size() == 2);

// Arguments that are themselves constants
constexpr int first           = 10;
constexpr int second          = 20;
constexpr auto from_variables = neutron::make_array<int>(first, second);
static_assert(from_variables[1] == 20);

void test_elements_are_mutable_at_runtime() {
    constexpr int expected_sum = 10 + 2 + 3 + 4 + 5;
    auto values                = neutron::make_array<int>(1, 2, 3, 4, 5);

    values[0]      = 10;
    const auto sum = std::accumulate(values.begin(), values.end(), 0);

    require(sum == expected_sum);
}

void test_string_views_are_iterable() {
    constexpr std::size_t expected_total = 5 + 4 + 5; // alpha, beta, gamma
    const auto names =
        neutron::make_array<std::string_view>("alpha", "beta", "gamma");

    std::size_t total = 0;
    for (const auto name : names) {
        total += name.size();
    }

    require(total == expected_total);
    require(names[1] == "beta");
}

} // namespace

int main() {
    test_elements_are_mutable_at_runtime();
    test_string_views_are_iterable();
    return neutron_test::result();
}
