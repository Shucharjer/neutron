#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Produces the same template with no arguments.
    static_assert(
        std::same_as<empty_type_list_t<std::tuple<int, char>>, std::tuple<>>);
    static_assert(
        std::same_as<empty_value_list_t<value_list<1, 2>>, value_list<>>);

    // Detects specializations without arguments.
    static_assert(is_empty_template_v<std::tuple<>>);
    static_assert(is_empty_template_v<value_list<>>);
    static_assert(!is_empty_template_v<std::tuple<int>>);

    return neutron_test::result();
}
