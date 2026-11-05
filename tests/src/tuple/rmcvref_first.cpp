#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/tuple.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Satisfied when `rmcvref_first<Ty>` accepts a tuple of type `Tup`.
template <typename Ty, typename Tup>
concept can_find =
    requires(Tup&& tup) { rmcvref_first<Ty>(std::forward<Tup>(tup)); };

/// Result type of `rmcvref_first<Ty>` applied to a tuple of type `Tup`.
template <typename Ty, typename Tup>
using result_t = decltype(rmcvref_first<Ty>(std::declval<Tup>()));

using mixed_tuple = std::tuple<const int&, std::string, char, int&>;

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
    std::is_same_v<result_t<std::string, mixed_tuple&>, std::string&>);
static_assert(std::is_same_v<
              result_t<std::string, const mixed_tuple&>, const std::string&>);
static_assert(
    std::is_same_v<result_t<std::string, mixed_tuple&&>, std::string&&>);
static_assert(std::is_same_v<result_t<char, const mixed_tuple&>, const char&>);
static_assert(
    noexcept(rmcvref_first<std::string>(std::declval<mixed_tuple&>())));

// Reference elements of a forwarding tuple keep their own category.
static_assert(std::is_same_v<
              result_t<std::string, std::tuple<std::string&&, int&>&&>,
              std::string&&>);
static_assert(
    std::is_same_v<
        result_t<std::string, std::tuple<std::string&&, int&>&>, std::string&>);

// shared_tuple is supported as well.
static_assert(
    std::is_same_v<
        result_t<std::string, shared_tuple<int, std::string>&>, std::string&>);

constexpr bool lookup_in_constexpr() {
    int value = 1;
    std::tuple<char, int&, long> tuple{ 'a', value, 3L };

    rmcvref_first<int>(tuple) = 5;

    return value == 5 && rmcvref_first<char>(tuple) == 'a' &&
           rmcvref_first<long>(tuple) == 3L;
}
static_assert(lookup_in_constexpr());

void test_finds_the_first_stripped_match() {
    int number = 1;
    std::string text{ "text" };
    mixed_tuple tuple{ number, text, 'c', number };

    require(&rmcvref_first<std::string>(tuple) == &std::get<1>(tuple));
    require(rmcvref_first<char>(tuple) == 'c');
}

void test_rvalue_tuple_moves_the_element_out() {
    int number = 1;
    std::string text{ "text" };
    mixed_tuple tuple{ number, text, 'c', number };

    rmcvref_first<std::string>(tuple) += "-updated";
    std::string moved = rmcvref_first<std::string>(std::move(tuple));

    require(moved == "text-updated");
    // The tuple holds a copy, so the source string is untouched.
    require(text == "text");
}

void test_forwarding_tuple_forwards_its_references() {
    std::string text{ "forwarded" };
    int number = 42;
    auto tuple = std::forward_as_tuple(std::move(text), number);

    std::string moved = rmcvref_first<std::string>(std::move(tuple));

    require(moved == "forwarded");
    require(&rmcvref_first<int>(tuple) == &number);
}

void test_shared_tuple_element_is_found() {
    shared_tuple<int, std::string> tuple{ 1, "shared" };

    require(&rmcvref_first<std::string>(tuple) == &tuple.get<1>());
    require(rmcvref_first<int>(tuple) == 1);
}

} // namespace

int main() {
    test_finds_the_first_stripped_match();
    test_rvalue_tuple_moves_the_element_out();
    test_forwarding_tuple_forwards_its_references();
    test_shared_tuple_element_is_found();
    return neutron_test::result();
}
