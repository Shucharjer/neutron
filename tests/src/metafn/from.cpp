#include <concepts>
#include <tuple>
#include <utility>
#include <type_traits>
#include <neutron/metafn.hpp>

using namespace neutron;

template <auto V>
using int_constant = std::integral_constant<int, V>;

int main() {
    static_assert(std::same_as<
                  type_list_from_value_t<int_constant, value_list<1, 2, 3>>,
                  type_list<
                      std::integral_constant<int, 1>,
                      std::integral_constant<int, 2>,
                      std::integral_constant<int, 3>>>);

    static_assert(std::same_as<
                  value_list_from_t<std::integer_sequence<int, 1, 2, 3>>,
                  value_list<1, 2, 3>>);

    constexpr auto t = tuple_from_value<value_list<1, 2, 3>>();
    static_assert(std::get<0>(t) == 1);
    static_assert(std::get<1>(t) == 2);
    static_assert(std::get<2>(t) == 3);

    return 0;
}
