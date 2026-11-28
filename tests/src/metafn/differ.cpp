#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // True only when the lists share no element.
    static_assert(type_list_all_differs_from_v<
                  type_list<int, char>, type_list<double, float>>);
    static_assert(!type_list_all_differs_from_v<
                  type_list<int, char>, type_list<double, char>>);

    return neutron_test::result();
}
