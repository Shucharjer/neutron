#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <concepts>
    #include <type_traits>
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // Erases elements matching a predicate.
    static_assert(std::same_as<
                  type_list_erase_if_t<
                      std::is_pointer, type_list<int, char*, double>>,
                  type_list<int, double>>);

    // Erases elements present in another list.
    static_assert(std::same_as<
                  type_list_erase_in_t<
                      type_list<int, char, double>, type_list<char, double>>,
                  type_list<int>>);

    return neutron_test::result();
}
