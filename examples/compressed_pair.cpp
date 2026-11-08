// A std::pair-like type whose empty members take no storage.
#include <functional>
#include <neutron/print.hpp>
#include <neutron/utility.hpp>

/// A limit together with its (stateless) comparator.
using threshold = neutron::compressed_pair<std::less<>, int>;

static_assert(sizeof(threshold) == sizeof(int));

int main() {
    threshold limit{ std::less<>{}, 10 };
    limit.second() += 5;

    for (const int value : { 3, 20 }) {
        const bool below = limit.first()(value, limit.second());
        neutron::println("{} < {}: {}", value, limit.second(), below);
    }
}
