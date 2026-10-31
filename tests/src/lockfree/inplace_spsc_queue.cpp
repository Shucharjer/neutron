#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <neutron/detail/lockfree/inplace_spsc_queue.hpp>

namespace {

template <typename Queue, typename... Args>
concept push_backable = requires(Queue& queue, Args&&... args) {
    queue.push_back(std::forward<Args>(args)...);
};

template <std::size_t Size>
concept valid_size =
    requires { typename neutron::inplace_spsc_queue<int, Size>; };

static_assert(push_backable<neutron::inplace_spsc_queue<int, 4>, int>);
static_assert(push_backable<
              neutron::inplace_spsc_queue<std::string, 4>, const std::string&>);
static_assert(push_backable<
              neutron::inplace_spsc_queue<std::string, 4>, std::size_t, char>);
static_assert(push_backable<
              neutron::inplace_spsc_queue<std::unique_ptr<int>, 4>,
              std::unique_ptr<int>>);
static_assert(!push_backable<
              neutron::inplace_spsc_queue<std::unique_ptr<int>, 4>,
              const std::unique_ptr<int>&>);
static_assert(valid_size<1> && valid_size<64>);
static_assert(!valid_size<0> && !valid_size<3>);

bool test_single_element_can_be_popped() {
    constexpr auto expected = 42;

    neutron::inplace_spsc_queue<int, 4> queue;
    if (!queue.empty()) {
        return false;
    }

    queue.push_back(expected);
    if (queue.empty() || queue.front() != expected) {
        return false;
    }

    queue.pop_front();
    return queue.empty();
}

bool test_every_slot_is_usable() {
    constexpr std::size_t size  = 8;
    constexpr std::size_t round = 5;

    neutron::inplace_spsc_queue<std::size_t, size> queue;
    std::size_t pushed = 0;
    std::size_t popped = 0;
    // Wrap around several times to cover the masked indices. Filling every
    // slot blocks forever if the queue holds less than `size` values.
    for (std::size_t r = 0; r != round; ++r) {
        while (pushed - popped != size) {
            queue.push_back(pushed);
            ++pushed;
        }
        for (std::size_t i = 0; i != (size / 2) + (r % 2); ++i) {
            if (queue.empty() || queue.front() != popped) {
                return false;
            }
            queue.pop_front();
            ++popped;
        }
    }
    while (!queue.empty()) {
        if (queue.front() != popped) {
            return false;
        }
        queue.pop_front();
        ++popped;
    }
    return popped == pushed;
}

bool test_move_only_values() {
    neutron::inplace_spsc_queue<std::unique_ptr<int>, 2> queue;
    queue.push_back(std::make_unique<int>(1));
    auto value = std::make_unique<int>(2);
    queue.push_back(std::move(value));
    if (value) {
        return false;
    }

    for (int expected = 1; expected != 3; ++expected) {
        if (queue.empty() || *queue.front() != expected) {
            return false;
        }
        queue.pop_front();
    }
    return queue.empty();
}

bool test_destructor_destroys_remaining_values() {
    const auto value = std::make_shared<int>(0);
    {
        neutron::inplace_spsc_queue<std::shared_ptr<int>, 4> queue;
        // Move the counters across the slot boundary before filling.
        for (int i = 0; i != 3; ++i) {
            queue.push_back(value);
            queue.pop_front();
        }
        for (int i = 0; i != 3; ++i) {
            queue.push_back(value);
        }
        if (value.use_count() != 4) {
            return false;
        }
    }
    return value.use_count() == 1;
}

bool test_values_cross_threads_in_order() {
    constexpr std::size_t value_count = 1 << 20;

    // A tiny queue forces both ends to wait on each other.
    neutron::inplace_spsc_queue<std::size_t, 8> queue;
    std::atomic<bool> valid = true;

    std::jthread consumer([&] {
        for (std::size_t expected = 0; expected != value_count;) {
            if (queue.empty()) {
                continue;
            }
            // Keep draining on mismatch so that the producer never blocks.
            if (queue.front() != expected) {
                valid.store(false, std::memory_order_relaxed);
            }
            queue.pop_front();
            ++expected;
        }
    });

    for (std::size_t value = 0; value != value_count; ++value) {
        queue.push_back(value);
    }
    consumer.join();

    return valid.load(std::memory_order_relaxed) && queue.empty();
}

} // namespace

int main() {
    if (!test_single_element_can_be_popped()) {
        return 1;
    }
    if (!test_every_slot_is_usable()) {
        return 2;
    }
    if (!test_move_only_values()) {
        return 3;
    }
    if (!test_destructor_destroys_remaining_values()) {
        return 4;
    }
    if (!test_values_cross_threads_in_order()) {
        return 5;
    }
    return 0;
}
