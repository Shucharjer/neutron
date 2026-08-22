#include <type_traits>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(type_list_requires_recurse_v<
                  std::is_integral, type_list<int, char>>);
    static_assert(!type_list_requires_recurse_v<
                  std::is_integral, type_list<int, double>>);

    return 0;
}
