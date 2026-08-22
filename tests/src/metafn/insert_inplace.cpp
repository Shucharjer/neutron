#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    // Appends `std::tuple<Ty>` when no matching `std::tuple<...>` is present.
    static_assert(std::same_as<
                  insert_type_list_inplace_t<
                      std::tuple, int, type_list<char, double>>,
                  type_list<char, double, std::tuple<int>>>);

    // Merges `Ty` into the existing `std::tuple<...>` element in place.
    static_assert(std::same_as<
                  insert_type_list_inplace_t<
                      std::tuple, int, type_list<std::tuple<char>, double>>,
                  type_list<std::tuple<char, int>, double>>);

    static_assert(std::same_as<
                  insert_tagged_type_list_inplace_t<
                      int, type_list<char, double>, float>,
                  type_list<char, double, tagged_type_list<int, float>>>);

    static_assert(std::same_as<
                  insert_tagged_type_list_inplace_t<
                      int, type_list<tagged_type_list<int, char>, double>, float>,
                  type_list<tagged_type_list<int, char, float>, double>>);

    static_assert(std::same_as<
                  insert_tagged_value_list_inplace_t<
                      int, type_list<char>, 42>,
                  type_list<char, tagged_value_list<int, 42>>>);

    static_assert(std::same_as<
                  insert_tagged_value_list_inplace_t<
                      int, type_list<tagged_value_list<int, 1>, char>, 42>,
                  type_list<tagged_value_list<int, 1, 42>, char>>);

    static_assert(std::same_as<
                  insert_range_tagged_value_list_inplace_t<
                      int, type_list<char>, tagged_value_list, 1, 2, 3>,
                  type_list<char, tagged_value_list<int, 1, 2, 3>>>);

    return 0;
}
