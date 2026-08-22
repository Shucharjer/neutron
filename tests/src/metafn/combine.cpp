#include <concepts>
#include <tuple>
#include <utility>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_conbine_t<
                      type_list<std::tuple<int>, std::tuple<char>>>,
                  type_list<std::tuple<int, char>>>);

    static_assert(std::same_as<
                  type_list_conbine_t<
                      type_list<
                          std::tuple<int>, std::tuple<char>,
                          std::pair<double, long>>>,
                  type_list<std::tuple<int, char>, std::pair<double, long>>>);

    return 0;
}
