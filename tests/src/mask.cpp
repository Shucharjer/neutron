#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <neutron/detail/mask.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Highest bit of std::size_t.
constexpr std::size_t top_bit =
    std::size_t{ 1 } << (std::numeric_limits<std::size_t>::digits - 1);

// Power-of-two detection
static_assert(!_single_bit<0>);
static_assert(_single_bit<1>);
static_assert(_single_bit<2>);
static_assert(!_single_bit<3>);
static_assert(_single_bit<4>);
static_assert(!_single_bit<6>);
static_assert(_single_bit<64>);
static_assert(!_single_bit<65>);
static_assert(_single_bit<4096>);
static_assert(_single_bit<top_bit>);
static_assert(!_single_bit<(std::numeric_limits<std::size_t>::max)()>);

// The result keeps the type of the argument.
static_assert(
    std::is_same_v<decltype(_uint_dev<8>(std::uint8_t{})), std::uint8_t>);
static_assert(
    std::is_same_v<decltype(_uint_dev<8>(std::uint16_t{})), std::uint16_t>);
static_assert(
    std::is_same_v<decltype(_uint_mod<7>(std::uint32_t{})), std::uint32_t>);
static_assert(
    std::is_same_v<decltype(_uint_mod<8>(std::uint64_t{})), std::uint64_t>);
static_assert(
    std::is_same_v<decltype(_uint_dev<3>(std::size_t{})), std::size_t>);
static_assert(noexcept(_uint_dev<8>(std::size_t{})));
static_assert(noexcept(_uint_mod<8>(std::size_t{})));

// Powers of two use shifts and masks.
static_assert(_uint_dev<1>(std::size_t{ 37 }) == 37);
static_assert(_uint_mod<1>(std::size_t{ 37 }) == 0);
static_assert(_uint_dev<8>(std::size_t{ 37 }) == 4);
static_assert(_uint_mod<8>(std::size_t{ 37 }) == 5);
static_assert(_uint_dev<4096>(std::size_t{ 8191 }) == 1);
static_assert(_uint_mod<4096>(std::size_t{ 8191 }) == 4095);
static_assert(_uint_dev<top_bit>(top_bit + 5) == 1);
static_assert(_uint_mod<top_bit>(top_bit + 5) == 5);

// Other divisors fall back to division.
static_assert(_uint_dev<3>(std::size_t{ 10 }) == 3);
static_assert(_uint_mod<3>(std::size_t{ 10 }) == 1);
static_assert(_uint_dev<100>(std::size_t{ 12345 }) == 123);
static_assert(_uint_mod<100>(std::size_t{ 12345 }) == 45);

// Narrow argument types
static_assert(_uint_dev<16>(std::uint8_t{ 255 }) == 15);
static_assert(_uint_mod<16>(std::uint8_t{ 255 }) == 15);
static_assert(_uint_dev<256>(std::uint8_t{ 255 }) == 0);
static_assert(_uint_mod<256>(std::uint8_t{ 255 }) == 255);
static_assert(_uint_mod<512>(std::uint8_t{ 200 }) == 200);
static_assert(_uint_dev<10>(std::uint16_t{ 65535 }) == 6553);
static_assert(_uint_dev<(1U << 16U)>(std::uint32_t{ 0x12345678 }) == 0x1234);
static_assert(_uint_mod<(1U << 16U)>(std::uint32_t{ 0x12345678 }) == 0x5678);

/// Compares `_uint_dev` and `_uint_mod` with `/` and `%` for `[0, limit)`.
template <std::size_t Divisor, typename Uint>
void check_matches_operators(Uint limit) {
    // Uses a bound unknown at compile time to exercise the runtime path.
    volatile Uint opaque       = limit;
    const Uint stop            = opaque;
    std::size_t dev_mismatches = 0;
    std::size_t mod_mismatches = 0;

    for (Uint value = 0; value < stop; ++value) {
        if (_uint_dev<Divisor>(value) != value / Divisor) {
            ++dev_mismatches;
        }
        if (_uint_mod<Divisor>(value) != value % Divisor) {
            ++mod_mismatches;
        }
    }

    require(dev_mismatches == 0);
    require(mod_mismatches == 0);
}

void test_size_t_matches_operators() {
    check_matches_operators<1>(std::size_t{ 300 });
    check_matches_operators<2>(std::size_t{ 300 });
    check_matches_operators<3>(std::size_t{ 300 });
    check_matches_operators<64>(std::size_t{ 5000 });
    check_matches_operators<100>(std::size_t{ 5000 });
}

void test_narrow_types_match_operators() {
    check_matches_operators<128>(std::uint16_t{ 60000 });
    check_matches_operators<7>(std::uint16_t{ 60000 });
    check_matches_operators<32>(std::uint8_t{ 255 });
    check_matches_operators<10>(std::uint8_t{ 255 });
    check_matches_operators<4096>(std::uint32_t{ 100000 });
}

void test_large_values_match_operators() {
    constexpr std::uint64_t low_half = 0xFFFF'FFFFULL;
    volatile std::uint64_t value     = 0xFFFF'FFFF'FFFF'FFFFULL;
    const std::uint64_t current      = value;

    require(_uint_dev<(1ULL << 32U)>(current) == low_half);
    require(_uint_mod<(1ULL << 32U)>(current) == low_half);
    require(_uint_dev<1000>(current) == current / 1000);
    require(_uint_mod<1000>(current) == current % 1000);
}

} // namespace

int main() {
    test_size_t_matches_operators();
    test_narrow_types_match_operators();
    test_large_values_match_operators();
    return neutron_test::result();
}
