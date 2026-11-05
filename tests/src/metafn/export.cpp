#include <concepts>
#include <tuple>
#include <variant>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Collects the arguments of matching specializations; others are dropped.
    static_assert(
        std::same_as<
            type_list_export_t<
                std::tuple,
                type_list<int, std::tuple<char>, std::tuple<double, float>>>,
            std::tuple<char, double, float>>);

    static_assert(
        std::same_as<
            type_list_export_as_t<
                std::tuple, type_list<int, std::tuple<char>>, std::variant>,
            std::variant<char>>);

    static_assert(
        std::same_as<
            value_list_export_t<
                value_list, type_list<value_list<1, 2>, int, value_list<3>>>,
            value_list<1, 2, 3>>);

    return neutron_test::result();
}
