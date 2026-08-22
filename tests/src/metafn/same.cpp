#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(type_list_has_same_template_v<
                  std::tuple<int>, std::tuple<char>>);
    static_assert(!type_list_has_same_template_v<int, std::tuple<char>>);

    return 0;
}
