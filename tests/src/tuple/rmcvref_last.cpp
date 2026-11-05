#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/tuple.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Satisfied when `rmcvref_last<Ty>` accepts a tuple of type `Tup`.
template <typename Ty, typename Tup>
concept can_find =
    requires(Tup&& tup) { rmcvref_last<Ty>(std::forward<Tup>(tup)); };

/// Result type of `rmcvref_last<Ty>` applied to a tuple of type `Tup`.
template <typename Ty, typename Tup>
using result_t = decltype(rmcvref_last<Ty>(std::declval<Tup>()));

using string_tuple = std::tuple<std::string, char, std::string>;

// Lookup ignores cv and reference qualifiers of the element types.
static_assert(can_find<int, std::tuple<const int&>&>);
static_assert(can_find<int, std::tuple<char, volatile int>&>);
static_assert(can_find<std::string, std::tuple<int, std::string&&>&>);
static_assert(can_find<int, const std::tuple<int>&>);
static_assert(can_find<int, std::tuple<int>>);

// The searched type is compared exactly against the stripped element types.
static_assert(!can_find<const int, std::tuple<const int>&>);
static_assert(!can_find<int&, std::tuple<int&>&>);
static_assert(!can_find<double, std::tuple<int, char>&>);
static_assert(!can_find<int, std::tuple<>&>);

// Value elements follow the value category of the tuple.
static_assert(
    std::is_same_v<result_t<std::string, string_tuple&>, std::string&>);
static_assert(std::is_same_v<
              result_t<std::string, const string_tuple&>, const std::string&>);
static_assert(
    std::is_same_v<result_t<std::string, string_tuple&&>, std::string&&>);
static_assert(
    noexcept(rmcvref_last<std::string>(std::declval<string_tuple&>())));

// The last stripped `int` here is the `int&&` element, seen as an lvalue.
static_assert(
    std::is_same_v<
        result_t<int, std::tuple<int, const int&, std::string, int&&>&>, int&>);

constexpr bool lookup_in_constexpr() {
    int first  = 1;
    int second = 2;
    std::tuple<int&, char, const int&, int&> tuple{ first, 'a', first, second };

    rmcvref_last<int>(tuple) = 5;

    return first == 1 && second == 5 && rmcvref_last<char>(tuple) == 'a';
}
static_assert(lookup_in_constexpr());

void test_duplicates_resolve_to_the_last_match() {
    int number = 1;
    std::tuple<int, const int&, std::string, int&&> tuple{ 0, number, "text",
                                                           std::move(number) };

    require(&rmcvref_last<int>(tuple) == &number);
    require(&rmcvref_last<std::string>(tuple) == &std::get<2>(tuple));
}

void test_refers_to_the_last_string() {
    string_tuple tuple{ "head", 'c', "tail" };

    require(&rmcvref_last<std::string>(tuple) == &std::get<2>(tuple));
}

void test_rvalue_tuple_moves_only_the_last_match() {
    string_tuple tuple{ "head", 'c', "tail" };

    std::string moved = rmcvref_last<std::string>(std::move(tuple));

    require(moved == "tail");
    require(std::get<0>(tuple) == "head");
}

void test_shared_tuple_element_is_found() {
    shared_tuple<std::string, int, std::string> tuple{ "a", 1, "b" };

    require(&rmcvref_last<std::string>(tuple) == &tuple.get<2>());
    require(rmcvref_last<std::string>(tuple) == "b");
    require(rmcvref_last<int>(tuple) == 1);
}

} // namespace

int main() {
    test_duplicates_resolve_to_the_last_match();
    test_refers_to_the_last_string();
    test_rvalue_tuple_moves_only_the_last_match();
    test_shared_tuple_element_is_found();
    return neutron_test::result();
}
