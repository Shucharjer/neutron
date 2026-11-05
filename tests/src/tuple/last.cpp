#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/tuple.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Minimal non-tuple `Template<Types...>`.
template <typename...>
struct pack {};

using string_tuple = std::tuple<std::string, int, std::string>;

// Indices on tuples with unique element types
static_assert(tuple_last_v<int, std::tuple<int>> == 0);
static_assert(tuple_last_v<int, std::tuple<int, char, double>> == 0);
static_assert(tuple_last_v<char, std::tuple<int, char, double>> == 1);
static_assert(tuple_last_v<double, std::tuple<int, char, double>> == 2);
static_assert(tuple_last<double, std::tuple<int, char, double>>::value == 2);

// Duplicates resolve to the last occurrence.
static_assert(tuple_last_v<int, std::tuple<int, char, int>> == 2);
static_assert(tuple_last_v<int, std::tuple<int, int, int, char>> == 2);
static_assert(tuple_last_v<char, std::tuple<char, int, char, int>> == 2);

// Matching is exact: cv and references are part of the type.
static_assert(tuple_last_v<int, std::tuple<int, const int, int&>> == 0);
static_assert(tuple_last_v<const int, std::tuple<int, const int, int&>> == 1);

// Any `Template<Types...>` is accepted.
static_assert(
    tuple_last_v<std::string, pack<std::string, int, std::string>> == 2);
static_assert(tuple_last_v<int, std::pair<int, int>> == 1);

// get_last keeps the constness of the tuple and never throws.
static_assert(std::is_same_v<
              decltype(get_last<std::string>(std::declval<string_tuple&>())),
              std::string&>);
static_assert(
    std::is_same_v<
        decltype(get_last<std::string>(std::declval<const string_tuple&>())),
        const std::string&>);
static_assert(noexcept(get_last<std::string>(std::declval<string_tuple&>())));
static_assert(
    noexcept(get_last<std::string>(std::declval<const string_tuple&>())));

constexpr bool access_in_constexpr() {
    std::tuple<int, char, int> tuple{ 1, 'a', 3 };

    get_last<int>(tuple) = 4;

    const auto& view = tuple;
    return get_last<int>(view) == 4 && std::get<0>(view) == 1 &&
           get_last<char>(view) == 'a';
}
static_assert(access_in_constexpr());

void test_get_last_refers_to_the_last_match() {
    string_tuple tuple{ "head", 1, "tail" };

    require(&get_last<std::string>(tuple) == &std::get<2>(tuple));
}

void test_get_last_writes_only_the_last_match() {
    string_tuple tuple{ "head", 1, "tail" };

    get_last<std::string>(tuple) += "-updated";
    get_last<int>(tuple) = 9;

    const auto& view = tuple;
    require(get_last<std::string>(view) == "tail-updated");
    require(std::get<0>(view) == "head");
    require(get_last<int>(view) == 9);
}

} // namespace

int main() {
    test_get_last_refers_to_the_last_match();
    test_get_last_writes_only_the_last_match();
    return neutron_test::result();
}
