#include <array>
#include <atomic>
#include <cstddef>
#include <future>
#include <neutron/detail/lockfree/intrusive_mpsc_queue.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

struct node {
    std::atomic<node*> next;
};

void test_pushes_from_other_threads_pop_in_order() {
    constexpr std::size_t count = 32;
    intrusive_mpsc_queue<&node::next> queue;
    std::array<node, count> nodes;

    // Pushes go through std::async, one at a time, so the order is fixed.
    for (std::size_t i = 0; i != count; ++i) {
        std::async([&queue, &nodes, i] {
            queue.push_back(&nodes[i]);
        }).get();
    }

    std::size_t popped = 0;
    while (node* const ptr = queue.pop_front()) {
        require_or_return(popped < count, );
        require(ptr == &nodes[popped]);
        ++popped;
    }
}

} // namespace

int main() {
    test_pushes_from_other_threads_pop_in_order();
    return neutron_test::result();
}
