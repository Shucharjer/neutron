#include <mutex>
#include <neutron/parallel.hpp>

using namespace neutron;

int main() {
    spinlock lock;
    std::lock_guard guard{ lock };

    return 0;
}
