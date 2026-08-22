#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_cat_t<
                      type_list<int, char>, type_list<double, float>>,
                  type_list<int, char, double, float>>);

    static_assert(std::same_as<
                  value_list_cat_t<value_list<1, 2>, value_list<3>>,
                  value_list<1, 2, 3>>);

    static_assert(std::same_as<
                  type_list_list_cat_t<
                      type_list<std::tuple<int, char>, std::tuple<double>>>,
                  type_list<std::tuple<int, char, double>>>);

    static_assert(std::same_as<
                  type_list_value_list_cat_t<
                      type_list<value_list<1, 2>, value_list<3>>>,
                  type_list<value_list<1, 2, 3>>>);

    static_assert(std::same_as<
                  tagged_type_list_cat_t<
                      tagged_type_list<int, char>,
                      tagged_type_list<int, double>>,
                  tagged_type_list<int, char, double>>);

    return 0;
}
