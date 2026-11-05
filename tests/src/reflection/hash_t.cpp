#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <neutron/reflection.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// Fixed-width unsigned types.
static_assert(std::same_as<hash_t, std::uint32_t>);
static_assert(std::same_as<long_hash_t, std::uint64_t>);
static_assert(std::numeric_limits<hash_t>::digits == 32);
static_assert(std::numeric_limits<long_hash_t>::digits == 64);
static_assert(!std::numeric_limits<hash_t>::is_signed);
static_assert(!std::numeric_limits<long_hash_t>::is_signed);

// The hashing helpers produce exactly these types.
static_assert(std::same_as<decltype(internal::hash("")), hash_t>);
static_assert(std::same_as<
              decltype(internal::hash_combine(std::array<hash_t, 1>{})),
              long_hash_t>);
static_assert(std::same_as<decltype(hash_of<int>()), hash_t>);

void test_hash_widens_to_long_hash_losslessly() {
    constexpr hash_t value = std::numeric_limits<hash_t>::max();

    constexpr long_hash_t wider = value;

    require(wider == 0xffffffffULL);
}

} // namespace

int main() {
    test_hash_widens_to_long_hash_losslessly();
    return neutron_test::result();
}
