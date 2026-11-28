#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <concepts>
    #include <tuple>
    #include <neutron/metafn.hpp>
#endif
#include "require.hpp"

using namespace neutron;

int main() {
    // Expands matching specializations one level.
    static_assert(std::same_as<
                  type_list_expose_t<
                      std::tuple, type_list<int, std::tuple<char, double>>>,
                  type_list<int, char, double>>);

    // Expands nested specializations recursively.
    static_assert(std::same_as<
                  type_list_recurse_expose_t<
                      std::tuple,
                      type_list<int, std::tuple<char, std::tuple<double>>>>,
                  type_list<int, char, double>>);

    // ignore_cvref strips references and cv-qualifiers.
    static_assert(std::same_as<ignore_cvref<const int&, int>::type, int>);

    return neutron_test::result();
}
