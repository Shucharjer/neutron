#include <concepts>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_pop_first_t<type_list<int, char, double>>,
                  type_list<char, double>>);

    return 0;
}
