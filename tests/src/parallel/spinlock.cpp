#include <mutex>
#include <neutron/parallel.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

void test_try_lock_succeeds_after_guard_unlocks() {
    spinlock lock;

    { const std::lock_guard guard{ lock }; }
    const std::unique_lock guard{ lock, std::try_to_lock };

    require(guard.owns_lock());
}

} // namespace

int main() {
    test_try_lock_succeeds_after_guard_unlocks();
    return neutron_test::result();
}
