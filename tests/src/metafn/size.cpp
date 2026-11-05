#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Number of elements.
    static_assert(type_list_size_v<type_list<int, char, double>> == 3);
    static_assert(type_list_size_v<type_list<>> == 0);
    static_assert(value_list_size_v<value_list<1, 2, 3>> == 3);

    return neutron_test::result();
}
