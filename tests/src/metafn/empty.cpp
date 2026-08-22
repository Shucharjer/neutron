#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  empty_type_list_t<std::tuple<int, char>>, std::tuple<>>);
    static_assert(std::same_as<
                  empty_value_list_t<value_list<1, 2>>, value_list<>>);

    static_assert(is_empty_template_v<std::tuple<>>);
    static_assert(is_empty_template_v<value_list<>>);
    static_assert(!is_empty_template_v<std::tuple<int>>);

    return 0;
}
