#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <neutron/interprocess.hpp>

using namespace neutron;

namespace {

// Unique per run so that parallel or aborted runs do not interfere.
// Kept short: macOS limits names to 31 characters.
std::string make_name(const char* tag) {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string("nt_") + tag + "_" +
           std::to_string(static_cast<std::uint32_t>(stamp));
}

template <typename Exception, typename Fn>
bool throws(Fn&& fn) {
    ATOM_TRY { std::forward<Fn>(fn)(); }
    ATOM_CATCH(const Exception&) { return true; }
    return false;
}

bool test_read_write_is_visible_to_read_only() {
    const auto name = make_name("rw");
    shared_memory shm(name, 64, read_write);
    if (shm.size() != 64 || shm.data() == nullptr) {
        return false;
    }
    static_cast<std::uint32_t*>(shm.data())[7] = 42;

    const shared_memory rshm(name, 64, read_only);
    if (rshm.span().size() != 64) {
        return false;
    }
    return static_cast<const std::uint32_t*>(rshm.data())[7] == 42;
}

bool test_open_existing_read_write() {
    const auto name = make_name("ex");
    shared_memory first(name, 32, read_write);
    std::memcpy(first.data(), "neutron", 8);

    // Reopening must neither resize nor clear the region.
    shared_memory second(name, 32, read_write);
    return std::memcmp(second.data(), "neutron", 8) == 0;
}

bool test_errors() {
    const auto name = make_name("er");
    if (!throws<std::system_error>([&] {
            shared_memory shm(name, 32, read_only);
        })) {
        return false;
    }
    if (!throws<std::invalid_argument>([&] {
            shared_memory shm(name, 0, read_write);
        })) {
        return false;
    }

    // Large enough to exceed page rounding on every platform.
    shared_memory shm(name, 32, read_write);
    return throws<std::runtime_error>([&] {
        shared_memory rshm(name, 1U << 20U, read_only);
    });
}

bool test_lifetime() {
    const auto name = make_name("lt");
    {
        shared_memory shm(name, 32, read_write);
        // Destroying a read-only view must not remove the region.
        { const shared_memory rshm(name, 32, read_only); }
        const shared_memory rshm(name, 32, read_only);
    }
    // The last owner is gone, so the name has been removed.
    if (!throws<std::system_error>([&] {
            shared_memory shm(name, 32, read_only);
        })) {
        return false;
    }

#if !defined(_WIN32)
    { shared_memory shm(name, 32, read_write, true); }
    { const shared_memory rshm(name, 32, read_only); }
    // Reclaim the kept region.
    { shared_memory shm(name, 32, read_write); }
#endif
    return true;
}

bool test_move() {
    const auto name = make_name("mv");
    shared_memory shm(name, 32, read_write);
    void* const addr = shm.data();

    shared_memory moved(std::move(shm));
    if (moved.data() != addr || shm.data() != nullptr || shm.size() != 0) {
        return false;
    }

    shared_memory other(make_name("mo"), 32, read_write);
    other = std::move(moved);
    if (other.data() != addr || moved.data() != nullptr) {
        return false;
    }

    // The region is still owned by `other` after the moves.
    const shared_memory rshm(name, 32, read_only);
    return rshm.data() != nullptr;
}

} // namespace

int main() {
    if (!test_read_write_is_visible_to_read_only()) {
        return 1;
    }
    if (!test_open_existing_read_write()) {
        return 2;
    }
    if (!test_errors()) {
        return 3;
    }
    if (!test_lifetime()) {
        return 4;
    }
    if (!test_move()) {
        return 5;
    }
    return 0;
}
