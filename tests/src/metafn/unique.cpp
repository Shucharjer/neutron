#include <concepts>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  unique_type_list_t<type_list<int, char, int, double, char>>,
                  type_list<int, char, double>>);

    static_assert(std::same_as<
                  unique_value_list_t<value_list<1, 2, 1, 3, 2>>,
                  value_list<1, 2, 3>>);

    return 0;
}
