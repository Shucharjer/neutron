#include <tuple>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Detects specializations of a given template.
    static_assert(is_specific_type_list_v<std::tuple, std::tuple<int>>);
    static_assert(!is_specific_type_list_v<std::tuple, int>);
    static_assert(is_specific_value_list_v<value_list, value_list<1>>);
    static_assert(!is_specific_value_list_v<value_list, int>);

    return neutron_test::result();
}
