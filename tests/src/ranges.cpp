#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
#include <neutron/ranges.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

template <typename First, typename Second>
using cpair = compressed_pair<First, Second>;

/// Forwards its arguments to a std::vector<int> constructor.
struct get_vector_fn {
    template <typename... Args>
    std::vector<int> operator()(Args&&... args) {
        return std::vector<int>(std::forward<Args>(args)...);
    }
};

/// Range adaptor closure returning an empty range of the input type.
struct empty_fn : range_adaptor_closure<empty_fn> {
    template <std::ranges::range Rng>
    Rng operator()(const Rng&) const {
        return Rng{};
    }
};

[[maybe_unused]] constexpr inline empty_fn empty;

std::map<int, int> make_map() {
    return {
        {   1,   5 },
        { 534,   2 },
        {  34, 756 },
        { 423,  87 },
        { 756,  15 }
    };
}

/// Characters of "word" including the terminating null.
std::array<char, sizeof("word")> make_word() {
    return { 'w', 'o', 'r', 'd', '\0' };
}

void test_to_vector_from_map_values() {
    const auto map = make_map();

    auto vector = ranges::to<std::vector>(map | std::views::values);

    // Values in key order.
    const std::vector<int> expected{ 5, 756, 87, 2, 15 };
    require(vector == expected);
}

void test_to_string_from_array() {
    auto array = make_word();

    auto string = ranges::to<std::string>(array);

    require(std::ranges::equal(string, array));
}

void test_to_closure_pipes_into_list() {
    auto array   = make_word();
    auto to_list = ranges::to<std::list>();

    auto list = array | to_list;

    require(std::ranges::equal(list, array));
}

void test_transform_after_to() {
    const auto map = make_map();
    auto vector    = ranges::to<std::vector>(map | std::views::values);

    auto transformed = vector | std::views::transform([](const auto& val) {
                           return std::to_string(val + 1);
                       });

    const std::vector<std::string> expected{ "6", "757", "88", "3", "16" };
    require(std::ranges::equal(transformed, expected));
}

void test_elements_of_map() {
    const std::initializer_list<std::pair<const int, int>> list = {
        {  423,   423 },
        { 4234, 23423 }
    };
    std::map<int, int> map = list;

    const std::vector<int> expected_keys{ 423, 4234 };
    const std::vector<int> expected_values{ 423, 23423 };
    require(std::ranges::equal(map | views::keys, expected_keys));
    require(std::ranges::equal(map | views::values, expected_values));
}

void test_elements_of_compressed_pairs() {
    // `vec | std::views::keys` is ill-formed for compressed_pair, but
    // neutron's views accept it.
    std::vector<cpair<int, std::string>> vec;

    auto keys   = vec | views::keys;
    auto values = vec | views::values;

    require(keys.begin() == keys.end());
    require(values.begin() == values.end());
}

#if __cplusplus <= 202002L
void test_closure_without_stored_arguments() {
    constexpr std::size_t construct_arg = 10;
    auto get_vector                     = make_closure<get_vector_fn>();

    auto result = get_vector(construct_arg);

    require(result.size() == construct_arg);
}

void test_user_closure_composes_with_std_adaptors() {
    std::vector vector = { 2, 3, 4, 4 };

    auto empty_vector = vector | empty;
    auto closure      = empty | std::views::reverse;
    // Composition of a composition must be well-formed.
    [[maybe_unused]] auto another_closure = closure | std::views::reverse;
    auto another_empty_vector             = vector | closure;

    require(empty_vector.empty());
    require(another_empty_vector.empty());
}

void test_closure_with_stored_end_iterator() {
    std::vector origin                  = { 2,   2,  3,   34, 2,    523, 53, 5,
                                            346, 54, 645, 7,  4567, 56,  75 };
    constexpr std::ptrdiff_t skip_two   = 2;
    constexpr std::ptrdiff_t skip_three = 3;
    auto closure = make_closure<get_vector_fn>(origin.cend());

    auto vec  = closure(origin.cbegin() + skip_two);
    auto vec2 = closure(origin.cbegin() + skip_three);
    // Only checks that the expression is well-formed: it composes the
    // iterator with the closure instead of invoking the closure.
    [[maybe_unused]] auto vec3 = (origin.cbegin() + skip_three) | closure;

    require(std::ranges::equal(vec, origin | std::views::drop(skip_two)));
    require(std::ranges::equal(vec2, origin | std::views::drop(skip_three)));
}
#endif

} // namespace

int main() {
    test_to_vector_from_map_values();
    test_to_string_from_array();
    test_to_closure_pipes_into_list();
    test_transform_after_to();
    test_elements_of_map();
    test_elements_of_compressed_pairs();
#if __cplusplus <= 202002L
    test_closure_without_stored_arguments();
    test_user_closure_composes_with_std_adaptors();
    test_closure_with_stored_end_iterator();
#endif
    return neutron_test::result();
}
