#include <algorithm>
#include <array>
#include <cassert>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <vector>
#include <neutron/print.hpp>
#include <neutron/ranges.hpp>

void test_to();
void test_element_view();
#if __cplusplus <= 202002L
void test_pipeline();
void test_closure();
#endif

int main() {
    test_to();
    test_element_view();
#if __cplusplus <= 202002L
    test_pipeline();
    test_closure();
#endif
}

using namespace neutron;

template <typename First, typename Second>
using cpair = compressed_pair<First, Second>;

struct get_vector_fn {
    template <typename... Args>
    std::vector<int> operator()(Args&&... args) {
        return std::vector<int>(std::forward<Args>(args)...);
    }
};

struct empty_fn : range_adaptor_closure<empty_fn> {

    template <std::ranges::range Rng>
    Rng operator()(const Rng& range) const {
        return Rng{};
    }
};

constexpr inline empty_fn empty;

constexpr auto print_each = [](const auto& value) { print("{} ", value); };
constexpr auto newline    = []() { println(""); };

void test_to() {
    std::map<int, int> map = {
        {   1,   5 },
        { 534,   2 },
        {  34, 756 },
        { 423,  87 },
        { 756,  15 }
    };
    auto vector = ranges::to<std::vector>(map | std::views::values);
    std::ranges::for_each(vector, print_each);
    newline();

    std::array<char, sizeof("word")> array = { 'w', 'o', 'r', 'd', '\0' };
    auto string                            = ranges::to<std::string>(array);
    print_each(string);
    newline();

    auto to_list = ranges::to<std::list>();

    auto list = array | to_list;
    std::ranges::for_each(list, print_each);
    newline();

    auto transformed = vector | std::views::transform([&](const auto& val) {
                           return std::to_string(val + 1);
                       });
    std::ranges::for_each(transformed, print_each);
    newline();
}

void test_element_view() {
    std::initializer_list<std::pair<const int, int>> list = {
        {  423,   423 },
        { 4234, 23423 }
    };

    std::map<int, int> map = list;
    std::ranges::for_each(map | views::keys, print_each);
    newline();
    std::ranges::for_each(map | views::values, print_each);
    newline();

    std::vector<cpair<int, std::string>> vec;

    // this is ill-formed:
    // auto sks = vec | std::views::keys;
    // but the fellow isn't.

    auto keys = vec | views::keys;
    std::ranges::for_each(keys, print_each);
    newline();
    auto vals = vec | views::values;
    std::ranges::for_each(vals, print_each);
    newline();
}
#if __cplusplus <= 202002L
void test_pipeline() {
    const auto construct_arg = 10;
    auto get_vector          = make_closure<get_vector_fn>();
    auto result              = get_vector(construct_arg);
    assert(result.size() == construct_arg);

    std::vector vector = { 2, 3, 4, 4 };
    auto empty_vector  = vector | empty;
    assert(empty_vector.empty());

    auto closure              = empty | std::views::reverse;
    auto another_closure      = closure | std::views::reverse;
    auto another_empty_vector = vector | closure;
    assert(another_empty_vector.empty());
}
void test_closure() {
    std::vector origin = { 2,   2,  3,   34, 2,    523, 53, 5,
                           346, 54, 645, 7,  4567, 56,  75 };
    auto end           = origin.cend();
    auto closure       = make_closure<get_vector_fn>(end);

    auto vec  = closure(origin.cbegin() + 2);
    auto vec2 = closure(origin.cbegin() + 3);
    auto vec3 = (origin.cbegin() + 3) | closure;
    std::ranges::for_each(vec, print_each);
    newline();
}
#endif
