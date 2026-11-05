#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/ranges.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

/// Aggregate exposing its members through a member get.
struct point {
    int x;
    int y;

    template <std::size_t Index>
    constexpr int& get() noexcept {
        return Index == 0 ? x : y;
    }

    template <std::size_t Index>
    constexpr const int& get() const noexcept {
        return Index == 0 ? x : y;
    }
};

template <>
struct std::tuple_size<point> : std::integral_constant<std::size_t, 2> {};

template <std::size_t Index>
struct std::tuple_element<Index, point> {
    using type = int;
};

namespace {

namespace nv = neutron::ranges::views;

using pair_vec  = std::vector<std::pair<int, std::string>>;
using tuple_vec = std::vector<std::tuple<int, std::string, double>>;
using str_map   = std::map<int, std::string>;

template <typename Rng, std::size_t Index>
using ev_t = nv::elements_view<std::views::all_t<Rng&>, Index>;

using pair_values = ev_t<pair_vec, 1>;
using map_keys    = ev_t<str_map, 0>;
using map_values  = ev_t<str_map, 1>;
using list_keys   = ev_t<std::list<std::pair<int, int>>, 0>;

template <typename View>
using iter_t = std::ranges::iterator_t<View>;

// Range concepts.
static_assert(std::ranges::range<pair_values>);
static_assert(std::ranges::input_range<pair_values>);
static_assert(std::ranges::forward_range<pair_values>);
static_assert(std::ranges::bidirectional_range<pair_values>);
static_assert(std::ranges::sized_range<pair_values>);
static_assert(std::ranges::common_range<pair_values>);
static_assert(std::ranges::bidirectional_range<map_keys>);
static_assert(std::ranges::sized_range<map_keys>);
static_assert(std::ranges::bidirectional_range<list_keys>);

// Iterator concepts.
static_assert(std::input_iterator<iter_t<pair_values>>);
static_assert(std::forward_iterator<iter_t<pair_values>>);
static_assert(std::bidirectional_iterator<iter_t<pair_values>>);
static_assert(std::sentinel_for<iter_t<pair_values>, iter_t<pair_values>>);
static_assert(std::bidirectional_iterator<iter_t<map_keys>>);
static_assert(std::bidirectional_iterator<iter_t<list_keys>>);
static_assert(std::is_nothrow_default_constructible_v<iter_t<pair_values>>);

// Reference and value types follow std::views::elements.
static_assert(
    std::same_as<std::ranges::range_reference_t<pair_values>, std::string&>);
static_assert(
    std::same_as<std::ranges::range_value_t<pair_values>, std::string>);
static_assert(
    std::same_as<std::ranges::range_reference_t<map_keys>, const int&>);
static_assert(std::same_as<std::ranges::range_value_t<map_keys>, int>);
static_assert(
    std::same_as<std::ranges::range_reference_t<map_values>, std::string&>);
static_assert(
    std::same_as<
        std::ranges::range_rvalue_reference_t<pair_values>, std::string&&>);
static_assert(std::same_as<
              std::ranges::range_reference_t<ev_t<const str_map, 1>>,
              const std::string&>);
static_assert(
    std::same_as<std::ranges::range_reference_t<ev_t<tuple_vec, 2>>, double&>);
static_assert(std::same_as<
              std::ranges::range_reference_t<pair_values>,
              std::ranges::range_reference_t<
                  decltype(std::declval<pair_vec&>() | std::views::values)>>);

// The adaptor produces the expected view type.
static_assert(std::same_as<
              decltype(std::declval<pair_vec&>() | nv::values), pair_values>);
static_assert(
    std::same_as<decltype(nv::values(std::declval<pair_vec&>())), pair_values>);
static_assert(std::same_as<
              decltype(std::declval<pair_vec>() | nv::values),
              nv::elements_view<std::ranges::owning_view<pair_vec>, 1>>);
static_assert(std::same_as<decltype(nv::keys), const nv::element_fn<0>>);
static_assert(std::same_as<decltype(nv::values), const nv::element_fn<1>>);

// The adaptor is constrained on gettible element types.
static_assert(std::is_invocable_v<const nv::element_fn<0>&, pair_vec&>);
static_assert(std::is_invocable_v<const nv::element_fn<1>&, str_map&>);
static_assert(
    !std::is_invocable_v<const nv::element_fn<0>&, std::vector<int>&>);

/// Pairs { 1, 2 }, { 3, 4 }, { 5, 6 } usable in constant evaluation.
constexpr std::array<std::pair<int, int>, 3> make_constexpr_pairs() {
    return {
        { { 1, 2 }, { 3, 4 }, { 5, 6 } }
    };
}

constexpr int constexpr_key_sum() {
    auto array = make_constexpr_pairs();
    int sum    = 0;
    for (const int key : array | nv::keys) {
        sum += key;
    }
    return sum;
}

constexpr int constexpr_value_sum() {
    auto array = make_constexpr_pairs();
    int sum    = 0;
    for (const int value : array | nv::values) {
        sum += value;
    }
    return sum;
}

static_assert(constexpr_key_sum() == 9);
static_assert(constexpr_value_sum() == 12);

pair_vec make_numbers() {
    return {
        { 3, "three" },
        { 1,   "one" },
        { 2,   "two" }
    };
}

void test_vector_matches_std_elements() {
    pair_vec vec = make_numbers();

    auto keys   = vec | nv::keys;
    auto values = vec | nv::values;

    require(std::ranges::equal(keys, vec | std::views::keys));
    require(std::ranges::equal(values, vec | std::views::values));
    require(std::ranges::equal(values, vec | std::views::elements<1>));
}

void test_vector_view_is_sized() {
    pair_vec vec             = make_numbers();
    const auto expected_size = vec.size();

    auto keys   = vec | nv::keys;
    auto values = vec | nv::values;

    require(keys.size() == expected_size);
    require(values.size() == expected_size);
    require(std::ranges::size(values) == expected_size);
    require(std::ranges::distance(values) == 3);
}

void test_elements_refer_into_base() {
    pair_vec vec = make_numbers();
    auto values  = vec | nv::values;

    require(&*values.begin() == &vec[0].second);

    for (auto& value : values) {
        value += "!";
    }

    require(vec[2].second == "two!");
}

tuple_vec make_tuples() {
    return {
        { 1,  "a", 0.5 },
        { 2, "bb", 1.5 },
        { 3,  "c", 2.5 }
    };
}

void test_tuple_matches_std_elements() {
    tuple_vec vec = make_tuples();

    auto first  = vec | nv::elements<0>;
    auto second = vec | nv::elements<1>;
    auto third  = vec | nv::elements<2>;

    require(std::ranges::equal(first, vec | std::views::elements<0>));
    require(std::ranges::equal(second, vec | std::views::elements<1>));
    require(std::ranges::equal(third, vec | std::views::elements<2>));
    require(*std::ranges::max_element(third) == 2.5);
}

void test_const_tuple_is_read_only() {
    tuple_vec vec    = make_tuples();
    const auto& cvec = vec;

    auto csecond = cvec | nv::elements<1>;

    static_assert(std::same_as<
                  std::ranges::range_reference_t<decltype(csecond)>,
                  const std::string&>);
    require(std::ranges::equal(csecond, cvec | std::views::elements<1>));
}

str_map make_map() {
    return {
        { 5,  "five" },
        { 1,   "one" },
        { 3, "three" }
    };
}

void test_map_matches_std_elements() {
    str_map map = make_map();

    auto keys   = map | nv::keys;
    auto values = map | nv::values;

    require(std::ranges::equal(keys, map | std::views::keys));
    require(std::ranges::equal(values, map | std::views::values));
    require(*keys.begin() == 1);
    require(std::ranges::is_sorted(keys));
}

void test_map_values_are_mutable() {
    str_map map = make_map();

    for (auto& value : map | nv::values) {
        value.append("?");
    }

    require(map.at(3) == "three?");
}

void test_const_map_keys() {
    const str_map map = make_map();

    auto ckeys = map | nv::keys;

    require(std::ranges::equal(ckeys, map | std::views::keys));
}

pair_vec make_letters() {
    return {
        { 0, "a" },
        { 1, "b" },
        { 2, "c" },
        { 3, "d" }
    };
}

void test_iterator_compares_and_measures() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;
    auto first   = values.begin();
    auto last    = values.end();

    require(first != last);
    require_false(first == last);
    require(std::ranges::distance(first, last) == 4);
}

void test_iterator_steps_both_ways() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;
    auto first   = values.begin();
    auto it      = first;

    require(*++it == "b");
    require(*it++ == "b");
    require(*it == "c");
    require(*--it == "b");
    require(*it-- == "b");
    require(it == first);
}

void test_iterator_random_access_helpers() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;
    auto it      = values.begin();

    // Provided for random-access bases.
    it += 3;
    require(*it == "d");
    it -= 2;
    require(*it == "b");
    require(*(it + 2) == "d");
    require(*(it - 1) == "a");
}

void test_iterator_next_and_prev() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;
    auto first   = values.begin();
    auto last    = values.end();

    auto end = last;
    --end;

    require(*end == "d");
    require(std::ranges::next(first, 2) != end);
    require(std::ranges::prev(last) == end);
}

void test_iter_move_moves_selected_element_only() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;

    std::string moved = std::ranges::iter_move(values.begin());

    require(moved == "a");
    require(vec[0].second.empty());
    require(vec[0].first == 0);
}

void test_iter_swap_swaps_selected_elements_only() {
    pair_vec vec = make_letters();
    auto values  = vec | nv::values;
    auto first   = values.begin();
    auto end     = std::ranges::prev(values.end());

    std::ranges::iter_swap(std::ranges::next(first), end);

    require(vec[1].second == "d");
    require(vec[3].second == "b");
    require(vec[1].first == 1);
    require(vec[3].first == 3);
}

void test_value_initialized_iterators_compare_equal() {
    require(iter_t<pair_values>{} == iter_t<pair_values>{});
}

void test_empty_vector() {
    pair_vec vec;

    auto values = vec | nv::values;

    require(values.begin() == values.end());
    require(values.size() == 0);
    require(std::ranges::distance(values) == 0);
}

void test_empty_map() {
    str_map map;

    auto keys = map | nv::keys;

    require(keys.begin() == keys.end());
    require(keys.size() == 0);
}

void test_empty_filtered_base() {
    pair_vec vec;

    auto empty_filtered = vec | std::views::filter([](const auto&) {
                              return true;
                          }) |
                          nv::keys;

    require(empty_filtered.begin() == empty_filtered.end());
}

void test_rvalue_container_is_owned() {
    auto values = pair_vec{
        { 1,  "x" },
        { 2, "yy" }
    } | nv::values;

    std::size_t total = 0;
    for (const auto& value : values) {
        total += value.size();
    }

    constexpr std::size_t total_length = 3;
    require_or_return(values.size() == 2, );
    require(total == total_length);
    require(*std::ranges::next(values.begin()) == "yy");
}

pair_vec make_four() {
    return {
        { 1,   "one" },
        { 2,   "two" },
        { 3, "three" },
        { 4,  "four" }
    };
}

void test_std_views_before_elements() {
    pair_vec vec = make_four();

    auto even_values = vec | std::views::filter([](const auto& pair) {
                           return pair.first % 2 == 0;
                       }) |
                       nv::values;
    auto reversed_keys = vec | std::views::reverse | nv::keys;
    auto first_two     = vec | std::views::take(2) | nv::keys;

    const std::vector<std::string> expected_even{ "two", "four" };
    const std::vector<int> expected_reversed{ 4, 3, 2, 1 };
    require(std::ranges::equal(even_values, expected_even));
    require(std::ranges::equal(reversed_keys, expected_reversed));
    require(std::ranges::distance(first_two) == 2);
}

void test_std_views_after_elements() {
    pair_vec vec = make_four();
    auto values  = vec | nv::values;

    auto lengths = values | std::views::transform([](const std::string& value) {
                       return value.size();
                   });
    auto reversed    = values | std::views::reverse;
    auto long_values = values | std::views::filter([](const std::string& v) {
                           return v.size() > 3;
                       });
    auto taken       = values | std::views::take(3) | std::views::drop(1);

    const std::vector<std::size_t> expected_lengths{ 3, 3, 5, 4 };
    const std::vector<std::string> expected_taken{ "two", "three" };
    require(std::ranges::equal(lengths, expected_lengths));
    require(
        std::ranges::equal(
            reversed, vec | std::views::values | std::views::reverse));
    require_or_return(long_values.begin() != long_values.end(), );
    require(*long_values.begin() == "three");
    require(std::ranges::equal(taken, expected_taken));
}

void test_algorithms_on_elements() {
    pair_vec vec = make_four();
    auto values  = vec | nv::values;
    auto keys    = vec | nv::keys;

    const auto keys_above_two = std::ranges::count_if(keys, [](int key) {
        return key > 2;
    });

    require(std::ranges::find(values, std::string("three")) != values.end());
    require(keys_above_two == 2);
    require(*std::ranges::max_element(keys) == 4);
}

void test_conversion_to_container() {
    pair_vec vec = make_four();

    auto copied = ranges::to<std::vector>(vec | nv::values);

    const std::vector<std::string> expected{ "one", "two", "three", "four" };
    require(copied == expected);
}

void test_member_get_elements() {
    // std::views::elements can not handle this element type.
    std::vector<point> points{
        { 1, 2 },
        { 3, 4 }
    };

    int xs = 0;
    int ys = 0;
    for (const int x : points | nv::keys) {
        xs += x;
    }
    for (int& y : points | nv::values) {
        ys += y;
        y = 0;
    }

    constexpr int x_sum = 1 + 3;
    constexpr int y_sum = 2 + 4;
    require(xs == x_sum);
    require(ys == y_sum);
    require(points[1].y == 0);
}

void test_compressed_pair_elements() {
    std::vector<compressed_pair<int, std::string>> pairs;
    pairs.emplace_back(1, "first");
    pairs.emplace_back(2, "second");

    auto keys   = pairs | nv::keys;
    auto values = pairs | nv::values;

    const std::vector<int> expected_keys{ 1, 2 };
    require(std::ranges::equal(keys, expected_keys));
    require(*std::ranges::next(values.begin()) == "second");
}

} // namespace

int main() {
    test_vector_matches_std_elements();
    test_vector_view_is_sized();
    test_elements_refer_into_base();
    test_tuple_matches_std_elements();
    test_const_tuple_is_read_only();
    test_map_matches_std_elements();
    test_map_values_are_mutable();
    test_const_map_keys();
    test_iterator_compares_and_measures();
    test_iterator_steps_both_ways();
    test_iterator_random_access_helpers();
    test_iterator_next_and_prev();
    test_iter_move_moves_selected_element_only();
    test_iter_swap_swaps_selected_elements_only();
    test_value_initialized_iterators_compare_equal();
    test_empty_vector();
    test_empty_map();
    test_empty_filtered_base();
    test_rvalue_container_is_owned();
    test_std_views_before_elements();
    test_std_views_after_elements();
    test_algorithms_on_elements();
    test_conversion_to_container();
    test_member_get_elements();
    test_compressed_pair_elements();
    return neutron_test::result();
}
