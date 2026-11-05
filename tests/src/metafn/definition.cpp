#include <concepts>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // List detection.
    static_assert(is_type_list_v<type_list<int, char>>);
    static_assert(!is_type_list_v<int>);
    static_assert(is_value_list_v<value_list<1, 2>>);
    static_assert(!is_value_list_v<int>);

    // decayed_type_list decays every element.
    static_assert(std::same_as<
                  decayed_type_list<const int&, char[3], void>,
                  type_list<int, char*, void>>);

    // The first argument of a tagged list is its tag.
    static_assert(std::same_as<tagged_type_list<int, char, double>::tag, int>);
    static_assert(std::same_as<
                  tagged_type_list<int, char, double>::type_list,
                  type_list<char, double>>);

    static_assert(std::same_as<tagged_value_list<int, 1, 2>::tag, int>);
    static_assert(std::same_as<
                  tagged_value_list<int, 1, 2>::value_list, value_list<1, 2>>);

    return neutron_test::result();
}
