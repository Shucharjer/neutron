#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <neutron/interprocess.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Size of the regions whose content does not matter.
constexpr std::size_t region_size = 32;

/// Unique name per run, so that parallel or aborted runs do not interfere.
/// Kept short: macOS limits names to 31 characters.
std::string make_name(const char* tag) {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string("nt_") + tag + "_" +
           std::to_string(static_cast<std::uint32_t>(stamp));
}

/// True if `fn` throws an `Exception`.
template <typename Exception, typename Fn>
bool throws(Fn&& fn) {
    ATOM_TRY { std::forward<Fn>(fn)(); }
    ATOM_CATCH(const Exception&) { return true; }
    return false;
}

void test_read_write_is_visible_to_read_only() {
    constexpr std::size_t size    = 64;
    constexpr std::size_t index   = 7;
    constexpr std::uint32_t value = 42;
    const auto name               = make_name("rw");
    shared_memory shm(name, size, read_write);
    require(shm.size() == size);
    require_or_return(shm.data() != nullptr, );

    static_cast<std::uint32_t*>(shm.data())[index] = value;
    const shared_memory rshm(name, size, read_only);

    require(rshm.span().size() == size);
    require(static_cast<const std::uint32_t*>(rshm.data())[index] == value);
}

void test_reopening_read_write_keeps_content() {
    constexpr char text[] = "neutron"; // NOLINT(modernize-avoid-c-arrays)
    const auto name       = make_name("ex");
    shared_memory first(name, region_size, read_write);
    std::memcpy(first.data(), text, sizeof(text));

    // Reopening must neither resize nor clear the region.
    const shared_memory second(name, region_size, read_write);

    require(std::memcmp(second.data(), text, sizeof(text)) == 0);
}

void test_opening_missing_region_read_only_throws() {
    const auto name = make_name("er");

    require(throws<std::system_error>([&] {
        const shared_memory shm(name, region_size, read_only);
    }));
}

void test_zero_size_throws_invalid_argument() {
    const auto name = make_name("er");

    require(throws<std::invalid_argument>([&] {
        const shared_memory shm(name, 0, read_write);
    }));
}

void test_read_only_view_larger_than_region_throws() {
    // Large enough to exceed page rounding on every platform.
    constexpr std::size_t oversized = 1U << 20U;
    const auto name                 = make_name("er");
    const shared_memory shm(name, region_size, read_write);

    require(throws<std::runtime_error>([&] {
        const shared_memory rshm(name, oversized, read_only);
    }));
}

void test_last_owner_removes_region() {
    const auto name = make_name("lt");
    {
        const shared_memory shm(name, region_size, read_write);
        // Destroying a read-only view must not remove the region.
        { const shared_memory rshm(name, region_size, read_only); }
        const shared_memory rshm(name, region_size, read_only);
    }

    require(throws<std::system_error>([&] {
        const shared_memory shm(name, region_size, read_only);
    }));
}

// Passes by not throwing: the kept region is still there to open.
void test_kept_region_survives_its_owner() {
#if !defined(_WIN32)
    const auto name = make_name("lt");

    { const shared_memory shm(name, region_size, read_write, true); }
    { const shared_memory rshm(name, region_size, read_only); }

    // Reclaim the kept region.
    { const shared_memory shm(name, region_size, read_write); }
#endif
}

void test_move_construction_transfers_mapping() {
    const auto name = make_name("mv");
    shared_memory shm(name, region_size, read_write);
    void* const addr = shm.data();

    const shared_memory moved(std::move(shm));

    require(moved.data() == addr);
    // NOLINTBEGIN(bugprone-use-after-move)
    require(shm.data() == nullptr);
    require(shm.size() == 0);
    // NOLINTEND(bugprone-use-after-move)
}

void test_move_assignment_transfers_ownership() {
    const auto name = make_name("mv");
    shared_memory shm(name, region_size, read_write);
    void* const addr = shm.data();
    shared_memory moved(std::move(shm));
    shared_memory other(make_name("mo"), region_size, read_write);

    other = std::move(moved);

    require(other.data() == addr);
    require(moved.data() == nullptr); // NOLINT(bugprone-use-after-move)
    // The region is still owned by `other` after the moves.
    const shared_memory rshm(name, region_size, read_only);
    require(rshm.data() != nullptr);
}

} // namespace

int main() {
    test_read_write_is_visible_to_read_only();
    test_reopening_read_write_keeps_content();
    test_opening_missing_region_read_only_throws();
    test_zero_size_throws_invalid_argument();
    test_read_only_view_larger_than_region_throws();
    test_last_owner_removes_region();
    test_kept_region_survives_its_owner();
    test_move_construction_transfers_mapping();
    test_move_assignment_transfers_ownership();
    return neutron_test::result();
}
