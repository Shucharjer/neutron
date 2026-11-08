// Map unsigned ids to values with O(1) lookup and densely packed values.
#include <cstdint>
#include <string>
#include <neutron/print.hpp>
#include <neutron/sparse_map.hpp>

// It also works in constant expressions.
static_assert([] {
    neutron::sparse_map<std::uint32_t, int> map;
    map.emplace(7, 49);
    return map.at(7);
}() == 49);

int main() {
    neutron::sparse_map<std::uint32_t, std::string> names;
    names.emplace(3, "three");
    names.emplace(1024, "kilo");
    names.emplace(42, "answer");
    names.erase(3);

    neutron::println("42 -> {}", names.at(42));
    neutron::println("contains 3: {}", names.contains(3));

    // Iteration walks the dense storage, not the sparse id space.
    for (const auto& [id, name] : names) {
        neutron::println("{} -> {}", std::uint32_t{ id }, name);
    }
}
