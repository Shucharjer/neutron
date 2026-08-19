#include <cstddef>
#include <neutron/string.hpp>

using namespace neutron;

template <std::size_t Size>
constexpr void test_short_string() {
    using string_t = short_string<Size>;

    string_t string;
}

int main() {
    test_short_string<0>();
    test_short_string<8>();
    test_short_string<16>();
    test_short_string<32>();
    test_short_string<64>();

    return 0;
}
