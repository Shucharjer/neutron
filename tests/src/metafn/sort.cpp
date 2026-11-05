#include <concepts>
#include <type_traits>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

template <typename A, typename B>
using smaller = std::bool_constant<(sizeof(A) < sizeof(B))>;

int main() {
    // Sorts by a binary comparison metafunction.
    static_assert(std::same_as<
                  type_list_sort_t<smaller, type_list<char, int, short>>,
                  type_list<char, short, int>>);

    return neutron_test::result();
}
