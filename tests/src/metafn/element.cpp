#include <concepts>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Indexed access is zero-based.
    static_assert(std::same_as<
                  type_list_element_t<0, type_list<int, char, double>>, int>);
    static_assert(
        std::same_as<
            type_list_element_t<2, type_list<int, char, double>>, double>);

    static_assert(value_list_element_v<0, value_list<1, 2, 3>> == 1);
    static_assert(value_list_element_v<2, value_list<1, 2, 3>> == 3);

    return neutron_test::result();
}
