#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

constexpr std::size_t u8_max  = (std::numeric_limits<std::uint8_t>::max)();
constexpr std::size_t u16_max = (std::numeric_limits<std::uint16_t>::max)();
constexpr std::size_t u32_max = (std::numeric_limits<std::uint32_t>::max)();
constexpr std::size_t sz_max  = (std::numeric_limits<std::size_t>::max)();

/// Largest value representable by `T`.
template <typename T>
constexpr auto max_of = (std::numeric_limits<T>::max)();

/// Checks the size-independent guarantees of every alias for `N`.
template <std::size_t N>
constexpr bool holds() {
    using uint_t      = packed_uint_t<N>;
    using size_t_     = packed_size_t<N>;
    using fast_uint_t = fast_packed_uint_t<N>;
    using fast_size_t = fast_packed_size_t<N>;

    static_assert(std::is_unsigned_v<uint_t>);
    static_assert(std::is_unsigned_v<size_t_>);
    // `N` itself fits.
    static_assert(max_of<uint_t> >= N);
    static_assert(max_of<fast_uint_t> >= N);
    static_assert(max_of<fast_size_t> >= N);
    // Sizes keep a spare value above `N`, unless `N` is already the maximum.
    static_assert(max_of<size_t_> > N - (N == sz_max));
    // The fast variants are never narrower.
    static_assert(sizeof(fast_uint_t) >= sizeof(uint_t));
    static_assert(sizeof(fast_size_t) >= sizeof(size_t_));
    return true;
}

// `packed_uint_t<N>` is the smallest type able to hold the value `N`.
static_assert(std::is_same_v<packed_uint_t<0>, std::uint8_t>);
static_assert(std::is_same_v<packed_uint_t<1>, std::uint8_t>);
static_assert(std::is_same_v<packed_uint_t<u8_max>, std::uint8_t>);
static_assert(std::is_same_v<packed_uint_t<u8_max + 1>, std::uint16_t>);
static_assert(std::is_same_v<packed_uint_t<u16_max>, std::uint16_t>);
static_assert(std::is_same_v<packed_uint_t<u16_max + 1>, std::uint32_t>);
static_assert(std::is_same_v<packed_uint_t<u32_max>, std::uint32_t>);
static_assert(std::is_same_v<packed_uint_t<u32_max + 1>, std::uint64_t>);
static_assert(std::is_same_v<packed_uint_t<sz_max>, std::uint64_t>);

// `packed_size_t<N>` keeps the maximum value free, e.g. for a sentinel.
static_assert(std::is_same_v<packed_size_t<0>, std::uint8_t>);
static_assert(std::is_same_v<packed_size_t<u8_max - 1>, std::uint8_t>);
static_assert(std::is_same_v<packed_size_t<u8_max>, std::uint16_t>);
static_assert(std::is_same_v<packed_size_t<u16_max - 1>, std::uint16_t>);
static_assert(std::is_same_v<packed_size_t<u16_max>, std::uint32_t>);
static_assert(std::is_same_v<packed_size_t<u32_max - 1>, std::uint32_t>);
static_assert(std::is_same_v<packed_size_t<u32_max>, std::uint64_t>);
static_assert(std::is_same_v<packed_size_t<sz_max>, std::uint64_t>);

// The fast variants follow the same thresholds.
static_assert(std::is_same_v<fast_packed_uint_t<u8_max>, std::uint_fast8_t>);
static_assert(
    std::is_same_v<fast_packed_uint_t<u8_max + 1>, std::uint_fast16_t>);
static_assert(std::is_same_v<fast_packed_uint_t<u16_max>, std::uint_fast16_t>);
static_assert(
    std::is_same_v<fast_packed_uint_t<u16_max + 1>, std::uint_fast32_t>);
static_assert(std::is_same_v<fast_packed_uint_t<u32_max>, std::uint_fast32_t>);
static_assert(
    std::is_same_v<fast_packed_uint_t<u32_max + 1>, std::uint_fast64_t>);

static_assert(
    std::is_same_v<fast_packed_size_t<u8_max - 1>, std::uint_fast8_t>);
static_assert(std::is_same_v<fast_packed_size_t<u8_max>, std::uint_fast16_t>);
static_assert(
    std::is_same_v<fast_packed_size_t<u16_max - 1>, std::uint_fast16_t>);
static_assert(std::is_same_v<fast_packed_size_t<u16_max>, std::uint_fast32_t>);
static_assert(
    std::is_same_v<fast_packed_size_t<u32_max - 1>, std::uint_fast32_t>);
static_assert(std::is_same_v<fast_packed_size_t<u32_max>, std::uint_fast64_t>);

// Generic guarantees around every threshold
static_assert(holds<0>());
static_assert(holds<1>());
static_assert(holds<100>());
static_assert(holds<u8_max - 1>());
static_assert(holds<u8_max>());
static_assert(holds<u8_max + 1>());
static_assert(holds<u16_max - 1>());
static_assert(holds<u16_max>());
static_assert(holds<u16_max + 1>());
static_assert(holds<u32_max - 1>());
static_assert(holds<u32_max>());
static_assert(holds<u32_max + 1>());
static_assert(holds<sz_max - 1>());
static_assert(holds<sz_max>());

/// Stores `N` without truncation and keeps one spare value for sizes.
template <std::size_t N>
void check_round_trip() {
    // volatile keeps the stores from being folded away.
    volatile packed_uint_t<N> value = static_cast<packed_uint_t<N>>(N);
    volatile packed_size_t<N> size  = static_cast<packed_size_t<N>>(N);

    const packed_size_t<N> sentinel = max_of<packed_size_t<N>>;

    require(static_cast<std::size_t>(value) == N);
    require(static_cast<std::size_t>(size) == N);
    require(sentinel != size);
}

void test_values_round_trip_without_truncation() {
    check_round_trip<0>();
    check_round_trip<u8_max - 1>();
    check_round_trip<u8_max>();
    check_round_trip<u16_max>();
    check_round_trip<u32_max>();
    check_round_trip<u32_max + 1>();
}

} // namespace

int main() {
    test_values_round_trip_without_truncation();
    return neutron_test::result();
}
