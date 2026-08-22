#include <concepts>
#include <string>
#include <tuple>
#include <neutron/metafn.hpp>

using namespace neutron;

int main() {
    static_assert(std::same_as<
                  type_list_rebind_t<
                      std::tuple, type_list<int, char, double, std::string>>,
                  std::tuple<int, char, double, std::string>>);

    return 0;
}
