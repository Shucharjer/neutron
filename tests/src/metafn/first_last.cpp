#include <concepts>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_first_t<type_list<int, char, double>>, int>);
    static_assert(std::same_as<
                  type_list_last_t<type_list<int, char, double>>, double>);

    return 0;
}
