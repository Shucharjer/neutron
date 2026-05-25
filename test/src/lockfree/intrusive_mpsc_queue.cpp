#include <array>
#include <future>
#include <neutron/detail/lockfree/intrusive_mpsc_queue.hpp>

using namespace neutron;

struct node {
    std::atomic<node*> next;
};

int main() {
    intrusive_mpsc_queue<&node::next> queue;
    constexpr auto count = 32;
    std::array<node, count> nodes;
    std::array<node*, count> expect{};
    for (auto i = 0; i < count; ++i) {
        expect.at(i) = &nodes.at(i);
    }

    for (auto i = 0; i < count; ++i) {
        std::async([&queue, &nodes, i] {
            queue.push_back(&nodes.at(i));
        }).get();
    }

    auto idx = 0;
    while (node* ptr = queue.pop_front()) {
        if (ptr != expect.at(idx)) {
            return 1;
        }
        ++idx;
    }

    return 0;
}
