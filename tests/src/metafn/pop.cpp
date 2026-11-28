#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <concepts>
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // Removes the first element.
    static_assert(std::same_as<
                  type_list_pop_first_t<type_list<int, char, double>>,
                  type_list<char, double>>);

    return neutron_test::result();
}
