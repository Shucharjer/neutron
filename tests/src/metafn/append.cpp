#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  append_type_list_t<std::tuple, type_list<int, char>>,
                  type_list<int, char, std::tuple<>>>);

    static_assert(std::same_as<
                  append_value_list_t<value_list, type_list<int, char>>,
                  type_list<int, char, value_list<>>>);

    return 0;
}
