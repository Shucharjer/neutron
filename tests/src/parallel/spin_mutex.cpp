#include <mutex>
#include <neutron/parallel.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    spin_mutex mutex;
    { std::lock_guard guard{ mutex }; }
    {
        std::unique_lock guard{ mutex, std::try_to_lock };
        require_or_return(guard.owns_lock(), 1);
    }

    return 0;
}
