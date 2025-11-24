#include <string>
#include <tuple>
#include <neutron/auxiliary.hpp>

int main() {
    std::tuple<int, char, double, std::string> tup;
    auto& [i, ch, rest] = decompose<1, 1>(tup);

    //
    return 0;
}
