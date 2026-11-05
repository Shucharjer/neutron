#include <algorithm>
#include <array>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Map whose iteration order differs from its insertion order.
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

    require(std::ranges::equal(vector, map | std::views::values));
}

void test_result_composes_with_views() {
    const auto map = make_map();
    auto vector    = ranges::to<std::vector>(map | std::views::values);

    auto transformed = vector | std::views::transform([](const auto& val) {
                           return std::to_string(val + 1);
                       });

    // Values in key order, each incremented.
    const std::vector<std::string> expected{ "6", "757", "88", "3", "16" };
    require(std::ranges::equal(transformed, expected));
}

void test_to_string_from_array() {
    auto word = make_word();

    auto string = ranges::to<std::string>(word);

    require(std::ranges::equal(string, word));
}

void test_to_list_deduces_value_type() {
    auto word = make_word();

    auto list = ranges::to<std::list>(word);

    require(std::ranges::equal(list, word));
}

} // namespace

int main() {
    test_to_vector_from_map_values();
    test_result_composes_with_views();
    test_to_string_from_array();
    test_to_list_deduces_value_type();
    return neutron_test::result();
}
