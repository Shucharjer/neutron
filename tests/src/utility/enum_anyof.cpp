#include <cstdint>
#include <initializer_list>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Scoped single-bit flags, including the top bit.
enum class flags : std::uint8_t {
    read    = 1U << 0U,
    write   = 1U << 1U,
    execute = 1U << 2U,
    hidden  = 1U << 7U,
};

/// Unscoped enumeration with an explicit underlying type.
enum legacy : int {
    legacy_a = 0x1,
    legacy_b = 0x10,
    legacy_c = 0x100,
};

/// Uses the full width of a 64-bit underlying type.
enum class wide : std::uint64_t {
    low  = 1ULL << 0U,
    high = 1ULL << 63U,
};

/// Signed underlying type, with the sign bit as a flag.
enum class signed_flags : std::int8_t {
    one  = 1,
    sign = -128,
};

// Membership of single-bit enumerators
static_assert(enum_anyof(flags::read, { flags::read }));
static_assert(enum_anyof(flags::read, { flags::write, flags::read }));
static_assert(enum_anyof(flags::execute, { flags::read, flags::execute }));
static_assert(!enum_anyof(flags::write, { flags::read, flags::execute }));
static_assert(!enum_anyof(flags::hidden, { flags::read, flags::write }));
static_assert(enum_anyof(flags::hidden, { flags::hidden }));

// An empty candidate list never matches.
static_assert(!enum_anyof(flags::read, {}));

// Values are compared bitwise, so combined masks match any shared bit.
static_assert(enum_anyof(static_cast<flags>(0b011), { flags::write }));
static_assert(!enum_anyof(static_cast<flags>(0b011), { flags::execute }));
static_assert(
    enum_anyof(flags::read, { static_cast<flags>(0b101), flags::hidden }));

// Unscoped enums
static_assert(enum_anyof(legacy_c, { legacy_a, legacy_c }));
static_assert(!enum_anyof(legacy_b, { legacy_a, legacy_c }));

// The full width of the underlying type, including the sign bit
static_assert(enum_anyof(wide::high, { wide::low, wide::high }));
static_assert(!enum_anyof(wide::high, { wide::low }));
static_assert(enum_anyof(signed_flags::sign, { signed_flags::sign }));
static_assert(!enum_anyof(signed_flags::sign, { signed_flags::one }));

static_assert(noexcept(enum_anyof(flags::read, { flags::read })));

void test_runtime_values_match_listed_bits() {
    constexpr int bit_count        = 8;
    constexpr int expected_matches = 3; // read, execute and hidden
    // volatile keeps the values out of constant evaluation.
    volatile std::uint8_t raw = 0;
    int matches               = 0;

    for (int bit = 0; bit != bit_count; ++bit) {
        raw             = static_cast<std::uint8_t>(1U << bit);
        const auto flag = static_cast<flags>(raw);
        if (enum_anyof(flag, { flags::read, flags::execute, flags::hidden })) {
            ++matches;
        }
    }

    require(matches == expected_matches);
}

void test_named_initializer_list_is_accepted() {
    const auto list = { flags::write, flags::hidden };

    require(enum_anyof(flags::hidden, list));
    require_false(enum_anyof(flags::read, list));
}

} // namespace

int main() {
    test_runtime_values_match_listed_bits();
    test_named_initializer_list_is_accepted();
    return neutron_test::result();
}
