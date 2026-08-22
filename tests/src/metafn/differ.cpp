#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(type_list_all_differs_from_v<
                  type_list<int, char>, type_list<double, float>>);
    static_assert(!type_list_all_differs_from_v<
                  type_list<int, char>, type_list<double, char>>);

    return 0;
}
