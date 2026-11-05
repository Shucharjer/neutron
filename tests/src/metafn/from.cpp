#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

template <auto V>
using int_constant = std::integral_constant<int, V>;

int main() {
    // Builds a type list from values.
    static_assert(
        std::same_as<
            type_list_from_value_t<int_constant, value_list<1, 2, 3>>,
            type_list<
                std::integral_constant<int, 1>, std::integral_constant<int, 2>,
                std::integral_constant<int, 3>>>);

    // Builds a value list from an integer_sequence.
    static_assert(std::same_as<
                  value_list_from_t<std::integer_sequence<int, 1, 2, 3>>,
                  value_list<1, 2, 3>>);

    // Materializes the values as a tuple.
    constexpr auto t = tuple_from_value<value_list<1, 2, 3>>();
    static_assert(std::get<0>(t) == 1);
    static_assert(std::get<1>(t) == 2);
    static_assert(std::get<2>(t) == 3);

    return neutron_test::result();
}
