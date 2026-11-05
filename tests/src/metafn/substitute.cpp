#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // Replaces every occurrence of a type.
    static_assert(
        std::same_as<
            type_list_substitute_t<std::tuple<int, float, int>, int, long>,
            std::tuple<long, float, long>>);

    return neutron_test::result();
}
