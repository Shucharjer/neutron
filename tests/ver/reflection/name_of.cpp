#include <array>
#include <cstddef>
#include <iostream>
#include <map>
#include <optional>
#include <tuple>
#include <utility>

#include "neutron/reflection.hpp"

namespace test_types {

enum class color : unsigned char {
    red,
    blue
};

struct record {
    int value;
};

template <typename Ty, std::size_t Size = 2>
struct fixed {
    std::array<Ty, Size> values;
};

using alias = record;

} // namespace test_types

template <typename Ty>
void print_name(const char* label) {
    std::cout << label << ": " << neutron::name_of<Ty>() << '\n';
}

int main() {
    print_name<int>("int");
    print_name<const int>("const int");
    print_name<int*>("int pointer");
    print_name<int* const>("const int pointer");
    print_name<int&>("int reference");
    print_name<int[4]>("int array");
    print_name<void()>("function");
    print_name<int (*)(double)>("function pointer");
    print_name<test_types::color>("enum");
    print_name<test_types::record>("record");
    print_name<test_types::alias>("alias");
    print_name<std::array<int, 3>>("std array");
    print_name<std::pair<int, double>>("std pair");
    print_name<std::tuple<int, double, char>>("std tuple");
    print_name<std::optional<test_types::record>>("std optional");
    print_name<std::map<int, test_types::record>>("std map");
    print_name<std::array<std::pair<int, double>, 2>>("nested std array");
    print_name<test_types::fixed<int>>("default template argument");
    print_name<test_types::fixed<int, 4>>("explicit template argument");
    return 0;
}
