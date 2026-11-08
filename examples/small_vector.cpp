// A vector that keeps its first few elements inline, avoiding heap allocation.
#include <neutron/print.hpp>
#include <neutron/small_vector.hpp>

int main() {
    neutron::small_vector<int, 4> numbers{ 1, 2, 3 };
    neutron::println("capacity: {}", numbers.capacity()); // inline storage

    // Growing past the inline capacity moves the elements to the heap.
    numbers.push_back(4);
    numbers.push_back(5);
    neutron::println("capacity: {}", numbers.capacity());

    for (const int value : numbers) {
        neutron::print("{} ", value);
    }
    neutron::println("");
}
