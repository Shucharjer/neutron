#include <type_traits>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // True only when every element satisfies the predicate.
    static_assert(
        type_list_requires_recurse_v<std::is_integral, type_list<int, char>>);
    static_assert(!type_list_requires_recurse_v<
                  std::is_integral, type_list<int, double>>);

    return neutron_test::result();
}
