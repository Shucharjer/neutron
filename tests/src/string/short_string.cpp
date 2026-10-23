#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <neutron/string.hpp>

using namespace neutron;

template <std::size_t Size>
constexpr void test_short_string() {
    using string_t = short_string<Size>;

    string_t string;
}

void test_formatter() {
    short_string ss = "a short string";
    std::cout << std::format("{}", ss);
}

int main() {

    test_short_string<0>();
    test_short_string<8>();
    test_short_string<16>();
    test_short_string<32>();
    test_short_string<64>();

    {
        constexpr auto string = [] {
            short_string<16> str = "this is a string";
            std::array<char, 32> r{};

            return r;
        }();
    }

    test_formatter();

    return 0;
}
