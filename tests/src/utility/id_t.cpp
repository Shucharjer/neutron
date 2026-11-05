#include <atomic>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

#if __has_include(<sys/types.h>)
    #include <sys/types.h>
#endif

namespace {

// The width is selected by UTILS_LONG_ID.
#ifndef UTILS_LONG_ID
static_assert(std::is_same_v<neutron::id_t, std::uint32_t>);
#else
static_assert(std::is_same_v<neutron::id_t, std::uint64_t>);
#endif

// The alias is re-exported into the global namespace.
static_assert(std::is_same_v<::id_t, neutron::id_t>);

// Usable as a cheap, atomically shared counter
static_assert(std::is_unsigned_v<neutron::id_t>);
static_assert(std::is_trivially_copyable_v<neutron::id_t>);
static_assert(std::atomic<neutron::id_t>::is_always_lock_free);

void test_increment_past_max_wraps_to_zero() {
    neutron::id_t value = (std::numeric_limits<neutron::id_t>::max)();

    ++value;

    require(value == 0);
}

} // namespace

int main() {
    test_increment_past_max_wraps_to_zero();
    return neutron_test::result();
}
