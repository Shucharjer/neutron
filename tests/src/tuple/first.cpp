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

using string_tuple = std::tuple<int, std::string, double>;

// Indices on tuples with unique element types
static_assert(tuple_first_v<int, std::tuple<int>> == 0);
static_assert(tuple_first_v<int, std::tuple<int, char, double>> == 0);
static_assert(tuple_first_v<char, std::tuple<int, char, double>> == 1);
static_assert(tuple_first_v<double, std::tuple<int, char, double>> == 2);
static_assert(tuple_first<double, std::tuple<int, char, double>>::value == 2);

// Matching is exact: cv and references are part of the type.
static_assert(tuple_first_v<const int, std::tuple<int, const int, int&>> == 1);
static_assert(tuple_first_v<int&, std::tuple<int, const int, int&>> == 2);

// Any `Template<Types...>` is accepted.
static_assert(tuple_first_v<std::string, pack<int, std::string, char>> == 1);
static_assert(tuple_first_v<std::size_t, std::pair<int, std::size_t>> == 1);

// get_first keeps the constness of the tuple and never throws.
static_assert(std::is_same_v<
              decltype(get_first<std::string>(std::declval<string_tuple&>())),
              std::string&>);
static_assert(
    std::is_same_v<
        decltype(get_first<std::string>(std::declval<const string_tuple&>())),
        const std::string&>);
static_assert(noexcept(get_first<std::string>(std::declval<string_tuple&>())));
static_assert(
    noexcept(get_first<std::string>(std::declval<const string_tuple&>())));

constexpr bool access_in_constexpr() {
    std::tuple<int, char, long> tuple{ 1, 'a', 3L };

    get_first<char>(tuple) = 'b';

    const auto& view = tuple;
    return get_first<int>(view) == 1 && get_first<char>(view) == 'b' &&
           get_first<long>(view) == 3L;
}
static_assert(access_in_constexpr());

void test_get_first_refers_to_the_element() {
    string_tuple tuple{ 1, "first", 2.5 };
    const auto& view = tuple;

    require(&get_first<std::string>(tuple) == &std::get<1>(tuple));
    require(&get_first<double>(view) == &std::get<2>(tuple));
}

void test_get_first_writes_through() {
    string_tuple tuple{ 1, "first", 2.5 };

    get_first<std::string>(tuple) += "-updated";
    get_first<int>(tuple) = 7;

    const auto& view = tuple;
    require(get_first<std::string>(view) == "first-updated");
    require(get_first<int>(view) == 7);
    require(get_first<double>(view) == 2.5);
}

} // namespace

int main() {
    test_get_first_refers_to_the_element();
    test_get_first_writes_through();
    return neutron_test::result();
}
