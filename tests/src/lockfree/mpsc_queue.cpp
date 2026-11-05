#include <neutron/lockfree.hpp>
#include "require.hpp"

// mpsc_queue is still a draft, so this only checks that the header compiles.
// Planned first test:
//
// mpsc_queue<int> queue;
// constexpr auto count = 1024;
// std::jthread producer([&queue] {
//     for (auto i = 0; i < count; ++i) {
//         queue.push_back(i);
//     }
// });

int main() { return neutron_test::result(); }
