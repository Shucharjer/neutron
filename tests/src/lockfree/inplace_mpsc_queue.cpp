#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <neutron/detail/lockfree/inplace_mpsc_queue.hpp>
#include "require.hpp"

namespace {

/// Spins until `flag` is set, so that every thread starts together.
void wait_for(const std::atomic<bool>& flag) noexcept {
    while (!flag.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
}

template <typename Queue, typename Arg>
concept push_backable = requires(Queue& queue, Arg&& value) {
    queue.push_back(std::forward<Arg>(value));
};

// push_back accepts values and lvalue references.
static_assert(push_backable<neutron::inplace_mpsc_queue<int, 4>, int>);
static_assert(push_backable<
              neutron::inplace_mpsc_queue<std::string, 4>, const std::string&>);

void test_single_element_can_be_popped() {
    constexpr auto expected = 42;
    neutron::inplace_mpsc_queue<int, 4> queue;

    queue.push_back(expected);

    require_false(queue.empty());
    require(queue.front() == expected);

    queue.pop_front();

    require(queue.empty());
}

void test_multiple_producers_publish_each_value_once() {
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t value_count    = 512;
    constexpr std::size_t total_count    = producer_count * value_count;
    constexpr std::size_t capacity       = 64;

    neutron::inplace_mpsc_queue<std::size_t, capacity> queue;
    std::array<std::atomic<unsigned>, total_count> seen{};
    std::atomic<std::size_t> consumed = 0;
    std::atomic<bool> started         = false;
    std::atomic<bool> in_range        = true;

    std::jthread consumer([&] {
        wait_for(started);
        while (consumed.load(std::memory_order_relaxed) < total_count) {
            if (queue.empty()) {
                std::this_thread::yield();
                continue;
            }
            const auto value = queue.front();
            queue.pop_front();
            // Stop on a corrupted value, it cannot index `seen`.
            if (value >= total_count) {
                in_range.store(false, std::memory_order_relaxed);
                break;
            }
            std::next(seen.begin(), static_cast<std::ptrdiff_t>(value))
                ->fetch_add(1, std::memory_order_relaxed);
            consumed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::vector<std::jthread> producers;
    producers.reserve(producer_count);
    for (std::size_t producer = 0; producer != producer_count; ++producer) {
        producers.emplace_back([&, producer] {
            wait_for(started);
            const auto base = producer * value_count;
            for (std::size_t offset = 0; offset != value_count; ++offset) {
                queue.push_back(base + offset);
            }
        });
    }

    started.store(true, std::memory_order_release);
    producers.clear(); // Joins every producer.
    consumer.join();

    require(in_range.load(std::memory_order_relaxed));
    require(consumed.load(std::memory_order_relaxed) == total_count);
    for (const auto& counter : seen) {
        require(counter.load(std::memory_order_relaxed) == 1);
    }
    require(queue.empty());
}

} // namespace

int main() {
    test_single_element_can_be_popped();
    test_multiple_producers_publish_each_value_once();
    return neutron_test::result();
}
