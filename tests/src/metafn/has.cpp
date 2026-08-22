#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(type_list_has_v<type_list<int, char, double>, char>);
    static_assert(!type_list_has_v<type_list<int, char, double>, float>);

    static_assert(value_list_has_v<value_list<1, 2, 3>, 2>);
    static_assert(!value_list_has_v<value_list<1, 2, 3>, 5>);

    static_assert(tagged_type_list_has_tag_v<tagged_type_list<int, char>, int>);
    static_assert(!tagged_type_list_has_tag_v<tagged_type_list<int, char>, float>);
    static_assert(tagged_value_list_has_tag_v<tagged_value_list<int, 1>, int>);

    static_assert(tagged_list_has_tag_v<tagged_type_list<int, char>, int>);
    static_assert(tagged_list_has_tag_v<tagged_value_list<int, 1>, int>);

    static_assert(type_list_has_tag_v<
                  type_list<
                      tagged_type_list<int, char>,
                      tagged_value_list<float, 1>>, int>);
    static_assert(type_list_has_tag_v<
                  type_list<tagged_value_list<float, 1>>, float>);
    static_assert(!type_list_has_tag_v<
                  type_list<tagged_value_list<float, 1>>, int>);

    return 0;
}
