// A fixed-capacity vector stored inside the object, usable at compile time.
#include <algorithm>
#include <neutron/inplace_vector.hpp>
#include <neutron/print.hpp>

/// Collects the first eight primes.
constexpr auto first_primes() {
    neutron::inplace_vector<int, 8> primes;
    for (int candidate = 2; primes.size() != primes.capacity(); ++candidate) {
        auto divides = [candidate](int prime) {
            return candidate % prime == 0;
        };
        if (std::ranges::none_of(primes, divides)) {
            primes.push_back(candidate);
        }
    }
    return primes;
}

constexpr auto primes = first_primes();
static_assert(primes[7] == 19);

int main() {
    for (const int prime : primes) {
        neutron::print("{} ", prime);
    }
    neutron::println("");
}
