#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_expose_t<
                      std::tuple, type_list<int, std::tuple<char, double>>>,
                  type_list<int, char, double>>);

    static_assert(std::same_as<
                  type_list_recurse_expose_t<
                      std::tuple,
                      type_list<int, std::tuple<char, std::tuple<double>>>>,
                  type_list<int, char, double>>);

    static_assert(std::same_as<ignore_cvref<const int&, int>::type, int>);

    return 0;
}
