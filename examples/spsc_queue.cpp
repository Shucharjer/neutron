// Pass values from one thread to another through a lock-free queue.
#include <thread>
#include <neutron/lockfree.hpp>
#include <neutron/print.hpp>

int main() {
    constexpr int count = 1000;
    neutron::spsc_queue<int> queue(64); // grows on demand

    std::jthread producer([&queue] {
        for (int value = 1; value <= count; ++value) {
            queue.push_back(value);
        }
    });

    long long sum = 0;
    for (int received = 0; received != count;) {
        if (queue.empty()) {
            std::this_thread::yield();
            continue;
        }
        sum += queue.front();
        queue.pop_front();
        ++received;
    }
    neutron::println("sum: {}", sum);
}
