#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <tuple>
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // Detects specializations of the same template.
    static_assert(
        type_list_has_same_template_v<std::tuple<int>, std::tuple<char>>);
    static_assert(!type_list_has_same_template_v<int, std::tuple<char>>);

    return neutron_test::result();
}
