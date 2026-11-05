#include <cstddef>
#include <string>
#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Satisfied when `T` can be copy-list-initialised from `{}`.
template <typename T>
concept implicitly_default_constructible = requires(void (*fn)(T)) { fn({}); };

/// Sums the spread values.
template <auto... Values>
constexpr auto sum(value_spreader<Values...>) {
    return (0 + ... + Values);
}

/// Sums the sizes of the spread types.
template <typename... Types>
constexpr std::size_t total_size(type_spreader<Types...>) {
    return (std::size_t{ 0 } + ... + sizeof(Types));
}

/// Counts the spread types.
template <typename... Types>
constexpr std::size_t count(type_spreader<Types...>) {
    return sizeof...(Types);
}

/// Overloads selected by the number of spread types.
template <typename T>
std::string describe(type_spreader<T>) {
    return "one";
}

template <typename T, typename U, typename... Rest>
std::string describe(type_spreader<T, U, Rest...>) {
    return "many";
}

/// Overload deducing a single spread value.
template <auto Value>
std::string describe(value_spreader<Value>) {
    return std::to_string(Value);
}

// Spreaders are stateless tags.
static_assert(std::is_empty_v<value_spreader<>>);
static_assert(std::is_empty_v<value_spreader<1, 'a'>>);
static_assert(std::is_empty_v<type_spreader<>>);
static_assert(std::is_empty_v<type_spreader<int, std::string>>);
static_assert(std::is_trivially_copyable_v<value_spreader<1>>);
static_assert(std::is_trivially_copyable_v<type_spreader<int>>);

// The global instances have the expected types.
static_assert(std::is_same_v<decltype(spread_arg<>), const value_spreader<>>);
static_assert(
    std::is_same_v<
        decltype(spread_arg<1, 2U, 'c'>), const value_spreader<1, 2U, 'c'>>);
static_assert(std::is_same_v<decltype(spread_type<>), const type_spreader<>>);
static_assert(std::is_same_v<
              decltype(spread_type<int, const char&>),
              const type_spreader<int, const char&>>);

// Distinct arguments produce distinct types.
static_assert(!std::is_same_v<value_spreader<1>, value_spreader<1U>>);
static_assert(!std::is_same_v<value_spreader<1, 2>, value_spreader<2, 1>>);
static_assert(!std::is_same_v<type_spreader<int>, type_spreader<const int>>);

// The default constructor is explicit, so `{}` cannot be passed implicitly.
static_assert(std::is_default_constructible_v<value_spreader<1>>);
static_assert(std::is_default_constructible_v<type_spreader<int>>);
static_assert(!implicitly_default_constructible<value_spreader<1>>);
static_assert(!implicitly_default_constructible<type_spreader<int>>);

// The spread arguments are deduced back by function templates.
static_assert(sum(spread_arg<>) == 0);
static_assert(sum(spread_arg<1, 2, 3>) == 6);
static_assert(sum(spread_arg<1, 2L, 'a'>) == 1 + 2L + 'a');
static_assert(
    total_size(spread_type<char, short>) == sizeof(char) + sizeof(short));
static_assert(count(spread_type<>) == 0);
static_assert(count(spread_type<int, void, int&>) == 3);

void test_overloads_are_selected_by_type_count() {
    const auto copy = spread_type<std::string>;

    require(describe(spread_type<int>) == "one");
    require(describe(spread_type<int, double, char>) == "many");
    require(describe(copy) == "one");
}

void test_values_are_deduced_at_runtime() {
    require(describe(spread_arg<42>) == "42");
    require(describe(value_spreader<7>{}) == "7");
    require(sum(spread_arg<10, 20>) == 30);
}

} // namespace

int main() {
    test_overloads_are_selected_by_type_count();
    test_values_are_deduced_at_runtime();
    return neutron_test::result();
}
