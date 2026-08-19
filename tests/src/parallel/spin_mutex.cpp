#include <mutex>
#include <neutron/parallel.hpp>

using namespace neutron;

int main() {
    spin_mutex mutex;
    std::lock_guard guard{ mutex };

    return 0;
}
