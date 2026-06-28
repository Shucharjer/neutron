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

namespace {

template <typename Queue, typename Arg>
concept push_backable = requires(Queue& queue, Arg&& value) {
    queue.push_back(std::forward<Arg>(value));
};

static_assert(push_backable<neutron::inplace_mpsc_queue<int, 4>, int>);
static_assert(push_backable<
              neutron::inplace_mpsc_queue<std::string, 4>, const std::string&>);

bool test_single_element_can_be_popped() {
    constexpr auto expected = 42;

    neutron::inplace_mpsc_queue<int, 4> queue;

    queue.push_back(expected);
    if (queue.empty()) {
        return false;
    }
    if (queue.front() != expected) {
        return false;
    }

    queue.pop_front();
    return queue.empty();
}

bool test_multiple_producers_publish_each_value_once() {
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t value_count    = 512;
    constexpr std::size_t total_count    = producer_count * value_count;

    neutron::inplace_mpsc_queue<std::size_t, 64> queue;
    std::array<std::atomic<unsigned>, total_count> seen{};
    std::atomic<std::size_t> consumed = 0;
    std::atomic<bool> started         = false;
    std::atomic<bool> valid           = true;

    std::jthread consumer([&] {
        while (!started.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        while (consumed.load(std::memory_order_relaxed) < total_count) {
            if (queue.empty()) {
                std::this_thread::yield();
                continue;
            }

            const auto value = queue.front();
            queue.pop_front();
            if (value >= total_count) {
                valid.store(false, std::memory_order_relaxed);
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
            while (!started.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            const auto base = producer * value_count;
            for (std::size_t offset = 0; offset != value_count; ++offset) {
                queue.push_back(base + offset);
            }
        });
    }

    started.store(true, std::memory_order_release);
    producers.clear();

    if (!valid.load(std::memory_order_relaxed)) {
        return false;
    }
    if (consumed.load(std::memory_order_relaxed) != total_count) {
        return false;
    }
    for (const auto& counter : seen) {
        if (counter.load(std::memory_order_relaxed) != 1) {
            return false;
        }
    }
    return queue.empty();
}

} // namespace

int main() {
    if (!test_single_element_can_be_popped()) {
        return 1;
    }
    if (!test_multiple_producers_publish_each_value_once()) {
        return 2;
    }
    return 0;
}
