#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <concepts>
    #include <string>
    #include <tuple>
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // Moves the arguments into another template.
    static_assert(std::same_as<
                  type_list_rebind_t<
                      std::tuple, type_list<int, char, double, std::string>>,
                  std::tuple<int, char, double, std::string>>);

    return neutron_test::result();
}
