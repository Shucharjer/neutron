// An open-addressing hash map with a std::unordered_map-like interface.
#include <string>
#include <neutron/flat_hash_map.hpp>
#include <neutron/print.hpp>

int main() {
    neutron::flat_hash_map<std::string, int> ages{
        { "alice", 30 },
        {   "bob", 25 }
    };
    ages["carol"] = 41;
    ages.erase("bob");

    if (auto iter = ages.find("alice"); iter != ages.end()) {
        neutron::println("alice: {}", iter->second);
    }
    neutron::println("has bob: {}", ages.count("bob") != 0);

    for (const auto& [name, age] : ages) {
        neutron::println("{} is {}", name, age);
    }
}
