#include <concepts>
#include <type_traits>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_erase_if_t<
                      std::is_pointer, type_list<int, char*, double>>,
                  type_list<int, double>>);

    static_assert(std::same_as<
                  type_list_erase_in_t<
                      type_list<int, char, double>, type_list<char, double>>,
                  type_list<int>>);

    return 0;
}
