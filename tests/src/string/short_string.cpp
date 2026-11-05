#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <neutron/string.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

template <std::size_t Size>
constexpr void default_construct() {
    using string_t = short_string<Size>;

    string_t string;
}

// A literal that fills the whole capacity is accepted in constant evaluation.
[[maybe_unused]] constexpr auto full_capacity_literal = [] {
    short_string<16> str = "this is a string";
    std::array<char, 32> r{};

    return r;
}();

void test_default_constructible_for_every_capacity() {
    default_construct<0>();
    default_construct<8>();
    default_construct<16>();
    default_construct<32>();
    default_construct<64>();
}

void test_formatter_prints_the_contents() {
    short_string ss = "a short string";

    const auto text = std::format("{}", ss);

    std::cout << text;
    require(text == "a short string");
}

} // namespace

int main() {
    test_default_constructible_for_every_capacity();
    test_formatter_prints_the_contents();
    return neutron_test::result();
}
