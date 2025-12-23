#include <string>
#include <neutron/pair.hpp>

using namespace neutron;

int main() {
    auto cpair   = compressed_pair<int, int>{};
    auto& rcpair = reverse(cpair);
    auto another = compressed_pair<int, std::string>{};
    // cause error: std::string is not trivial type.
    // auto& another_reversed = utils::reverse(another);

    auto pcpair      = compated_pair<char, int>{};
    auto& [pcf, pcs] = pcpair;

    compressed_pair<int, int> third{ cpair.first(), cpair.second() };
}
