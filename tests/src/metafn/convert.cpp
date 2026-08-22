#include <concepts>
#include <variant>
#include <type_traits>
#include <neutron/metafn.hpp>

using namespace neutron;

template <auto V>
struct double_value {
    static constexpr auto type = V * 2;
};

template <auto V>
using int_constant = std::integral_constant<int, V>;

int main() {
    static_assert(std::same_as<
                  type_list_convert_t<std::add_pointer, type_list<int, char>>,
                  type_list<int*, char*>>);

    static_assert(std::same_as<
                  value_list_convert_t<double_value, value_list<1, 2, 3>>,
                  value_list<2, 4, 6>>);

    static_assert(std::same_as<
                  value_list_to_type_list_convert_t<
                      int_constant, value_list<1, 2>, std::variant>,
                  std::variant<
                      std::integral_constant<int, 1>,
                      std::integral_constant<int, 2>>>);

    static_assert(std::same_as<
                  tagged_list_convert_t<
                      std::add_pointer, tagged_type_list<int, char, double>>,
                  tagged_type_list<int, char*, double*>>);

    return 0;
}
