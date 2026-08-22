#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_substitute_t<std::tuple<int, float, int>, int, long>,
                  std::tuple<long, float, long>>);

    return 0;
}
