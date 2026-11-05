#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/tuple.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Default constructor that may throw.
struct throwing_default {
    throwing_default() noexcept(false) {}
};

/// Satisfied when `tuple.get<Index>()` is well-formed.
template <typename Tuple, std::size_t Index>
concept has_get = requires(Tuple& tuple) { tuple.template get<Index>(); };

/// Result type of `get<Index>()` called on a tuple of type `Self`.
template <typename Self, std::size_t Index>
using get_result = decltype(std::declval<Self>().template get<Index>());

using pair_t   = shared_tuple<int, std::string>;
using single_t = shared_tuple<std::string>;
using mixed_t  = shared_tuple<std::string, int, std::string>;

/// Length that does not fit into the small-string buffer.
constexpr std::size_t long_length = 64;

// Empty tuple
static_assert(std::is_empty_v<shared_tuple<>>);
static_assert(std::tuple_size_v<shared_tuple<>> == 0);

// Every element occupies its own destructive-interference line.
static_assert(alignof(shared_tuple<char>) == hdi_size);
static_assert(sizeof(shared_tuple<char>) == hdi_size);
static_assert(alignof(shared_tuple<char, char>) == hdi_size);
static_assert(sizeof(shared_tuple<char, char>) == 2 * hdi_size);
static_assert(sizeof(shared_tuple<int, char, double>) == 3 * hdi_size);
static_assert(sizeof(shared_tuple<std::string, int>) % hdi_size == 0);

// Tuple protocol
static_assert(std::tuple_size_v<shared_tuple<int>> == 1);
static_assert(std::tuple_size_v<shared_tuple<int, char, double>> == 3);
static_assert(std::is_same_v<
              std::tuple_element_t<0, shared_tuple<int, char, double>>, int>);
static_assert(
    std::is_same_v<
        std::tuple_element_t<2, shared_tuple<int, char, double>>, double>);
static_assert(
    std::is_same_v<
        std::tuple_element_t<1, shared_tuple<int, const char&>>, const char&>);

// noexcept follows the element constructors.
static_assert(std::is_nothrow_default_constructible_v<shared_tuple<int>>);
static_assert(std::is_nothrow_default_constructible_v<shared_tuple<int, long>>);
static_assert(
    !std::is_nothrow_default_constructible_v<shared_tuple<throwing_default>>);
static_assert(!std::is_nothrow_default_constructible_v<
              shared_tuple<int, throwing_default>>);
static_assert(!std::is_nothrow_constructible_v<
              shared_tuple<std::string, int>, const char*, int>);
static_assert(
    std::is_nothrow_constructible_v<shared_tuple<int, long>, int, long>);
static_assert(std::is_nothrow_move_constructible_v<shared_tuple<int, long>>);

// get() propagates the value category, for several and for one element.
static_assert(std::is_same_v<get_result<pair_t&, 0>, int&>);
static_assert(std::is_same_v<get_result<const pair_t&, 1>, const std::string&>);
static_assert(std::is_same_v<get_result<pair_t&&, 1>, std::string&&>);
static_assert(
    std::is_same_v<get_result<const pair_t&&, 1>, const std::string&&>);
static_assert(std::is_same_v<get_result<single_t&, 0>, std::string&>);
static_assert(
    std::is_same_v<get_result<const single_t&, 0>, const std::string&>);
static_assert(std::is_same_v<get_result<single_t&&, 0>, std::string&&>);
static_assert(
    std::is_same_v<get_result<const single_t&&, 0>, const std::string&&>);
static_assert(noexcept(std::declval<pair_t&>().get<1>()));

// Index bounds are constrained.
static_assert(has_get<shared_tuple<int>, 0>);
static_assert(!has_get<shared_tuple<int>, 1>);
static_assert(has_get<shared_tuple<int, int>, 1>);
static_assert(!has_get<shared_tuple<int, int>, 2>);

constexpr bool access_in_constexpr() {
    shared_tuple<int, long, char> tuple{ 1, 2L, 'c' };

    tuple.get<1>() = 5L;

    const auto& view = tuple;
    return view.get<0>() == 1 && view.get<1>() == 5L && view.get<2>() == 'c' &&
           get_first<long>(view) == 5L && get_last<char>(view) == 'c';
}
static_assert(access_in_constexpr());

constexpr bool value_initialization_in_constexpr() {
    shared_tuple<int, long> tuple{};
    shared_tuple<int> single{};
    return tuple.get<0>() == 0 && tuple.get<1>() == 0L && single.get<0>() == 0;
}
static_assert(value_initialization_in_constexpr());

/// Checks that the three elements start on separate interference lines.
template <typename Tuple>
void check_separate_lines(Tuple& tuple) {
    const auto first =
        reinterpret_cast<std::uintptr_t>(&tuple.template get<0>());
    const auto second =
        reinterpret_cast<std::uintptr_t>(&tuple.template get<1>());
    const auto third =
        reinterpret_cast<std::uintptr_t>(&tuple.template get<2>());

    require(first % hdi_size == 0);
    require(second % hdi_size == 0);
    require(third % hdi_size == 0);
    require(second - first >= hdi_size);
    require(third - second >= hdi_size);
}

void test_char_elements_are_on_separate_lines() {
    shared_tuple<char, char, char> chars{ 'a', 'b', 'c' };

    check_separate_lines(chars);
}

void test_string_elements_are_on_separate_lines() {
    mixed_t mixed{ "x", 1, "y" };

    check_separate_lines(mixed);
}

void test_constructs_each_element() {
    const std::string head(long_length, 'a');

    mixed_t tuple{ std::string(head), 2, "tail" };

    require(tuple.get<0>() == head);
    require(tuple.get<1>() == 2);
    require(tuple.get<2>() == "tail");
}

void test_copy_and_move_construction_keep_the_values() {
    const std::string head(long_length, 'a');
    mixed_t tuple{ std::string(head), 2, "tail" };
    const auto& view = tuple;

    mixed_t copy{ view };
    mixed_t moved{ std::move(copy) };

    require(moved.get<0>() == head);
    require(moved.get<2>() == "tail");
    require(tuple.get<0>() == head);
}

void test_rvalue_get_moves_the_element_out() {
    const std::string head(long_length, 'a');
    mixed_t tuple{ std::string(head), 2, "tail" };

    std::string stolen = std::move(tuple).get<0>();

    require(stolen == head);
}

void test_typed_lookup_refers_to_the_elements() {
    mixed_t tuple{ std::string(long_length, 'a'), 2, "tail" };
    const auto& view = tuple;

    get_first<int>(tuple) = 3;
    get_last<std::string>(tuple) += "!";

    require(tuple.get<1>() == 3);
    require(tuple.get<2>() == "tail!");
    require(&get_first<int>(view) == &tuple.get<1>());
    require(&get_last<std::string>(view) == &tuple.get<2>());
}

void test_single_element_tuple() {
    single_t single{ "only" };
    const auto& view = single;

    std::string moved = std::move(single).get<0>();

    require(moved == "only");
    require(&view.get<0>() == &single.get<0>());
    require(&get_first<std::string>(single) == &single.get<0>());
    require(&get_last<std::string>(view) == &single.get<0>());
}

void test_trailing_elements_are_value_initialized() {
    shared_tuple<std::string, std::string, int> tuple{ "head" };

    require(tuple.get<0>() == "head");
    require(tuple.get<1>().empty());
    require(tuple.get<2>() == 0);
}

void test_structured_bindings_refer_to_the_elements() {
    pair_t tuple{ 4, "four" };

    auto& [number, text] = tuple;
    number               = 5;
    text += "!";

    const auto& [cnumber, ctext] = tuple;
    require(cnumber == 5);
    require(ctext == "four!");
    require(&number == &tuple.get<0>());
}

} // namespace

int main() {
    test_char_elements_are_on_separate_lines();
    test_string_elements_are_on_separate_lines();
    test_constructs_each_element();
    test_copy_and_move_construction_keep_the_values();
    test_rvalue_get_moves_the_element_out();
    test_typed_lookup_refers_to_the_elements();
    test_single_element_tuple();
    test_trailing_elements_are_value_initialized();
    test_structured_bindings_refer_to_the_elements();
    return neutron_test::result();
}
