#include <mutex>
#include <neutron/parallel.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    spinlock lock;
    { std::lock_guard guard{ lock }; }
    {
        std::unique_lock guard{ lock, std::try_to_lock };
        require_or_return(guard.owns_lock(), 1);
    }

    return 0;
}
