#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <neutron/detail/lockfree/inplace_spsc_queue.hpp>
#include "require.hpp"

namespace {

template <typename Queue, typename... Args>
concept push_backable = requires(Queue& queue, Args&&... args) {
    queue.push_back(std::forward<Args>(args)...);
};

template <std::size_t Size>
concept valid_size =
    requires { typename neutron::inplace_spsc_queue<int, Size>; };

// push_back constructs in place and respects move-only types.
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

// The capacity must be a non-zero power of two.
static_assert(valid_size<1>);
static_assert(valid_size<64>);
static_assert(!valid_size<0>);
static_assert(!valid_size<3>);

void test_single_element_can_be_popped() {
    constexpr auto expected = 42;
    neutron::inplace_spsc_queue<int, 4> queue;
    require(queue.empty());

    queue.push_back(expected);

    require_false(queue.empty());
    require(queue.front() == expected);

    queue.pop_front();

    require(queue.empty());
}

void test_every_slot_is_usable() {
    constexpr std::size_t size   = 8;
    constexpr std::size_t rounds = 5;
    neutron::inplace_spsc_queue<std::size_t, size> queue;
    std::size_t pushed = 0;
    std::size_t popped = 0;

    // Wrap around several times to cover the masked indices. Filling every
    // slot blocks forever if the queue holds less than `size` values.
    for (std::size_t r = 0; r != rounds; ++r) {
        while (pushed - popped != size) {
            queue.push_back(pushed);
            ++pushed;
        }
        for (std::size_t i = 0; i != (size / 2) + (r % 2); ++i) {
            require_or_return(!queue.empty(), );
            require(queue.front() == popped);
            queue.pop_front();
            ++popped;
        }
    }
    while (!queue.empty()) {
        require(queue.front() == popped);
        queue.pop_front();
        ++popped;
    }

    require(popped == pushed);
}

void test_move_only_values_are_moved_in() {
    // Exactly full, so push_back never waits.
    constexpr int capacity = 2;
    neutron::inplace_spsc_queue<std::unique_ptr<int>, capacity> queue;
    auto value = std::make_unique<int>(2);

    queue.push_back(std::make_unique<int>(1));
    queue.push_back(std::move(value));

    require_false(value); // NOLINT(bugprone-use-after-move)
    for (int expected = 1; expected != capacity + 1; ++expected) {
        require_or_return(!queue.empty(), );
        require(*queue.front() == expected);
        queue.pop_front();
    }
    require(queue.empty());
}

void test_destructor_destroys_remaining_values() {
    constexpr int count = 3;
    const auto value    = std::make_shared<int>(0);
    {
        neutron::inplace_spsc_queue<std::shared_ptr<int>, 4> queue;
        // Move the indices across the slot boundary before filling.
        for (int i = 0; i != count; ++i) {
            queue.push_back(value);
            queue.pop_front();
        }
        for (int i = 0; i != count; ++i) {
            queue.push_back(value);
        }

        require(value.use_count() == count + 1);
    }

    require(value.use_count() == 1);
}

void test_values_cross_threads_in_order() {
    constexpr std::size_t value_count = 1 << 20;
    // A tiny queue forces both ends to wait on each other.
    constexpr std::size_t capacity = 8;
    neutron::inplace_spsc_queue<std::size_t, capacity> queue;
    std::atomic<bool> in_order = true;

    std::jthread consumer([&] {
        for (std::size_t expected = 0; expected != value_count;) {
            if (queue.empty()) {
                continue;
            }
            // Keep draining on mismatch so that the producer never blocks.
            if (queue.front() != expected) {
                in_order.store(false, std::memory_order_relaxed);
            }
            queue.pop_front();
            ++expected;
        }
    });
    for (std::size_t value = 0; value != value_count; ++value) {
        queue.push_back(value);
    }
    consumer.join();

    require(in_order.load(std::memory_order_relaxed));
    require(queue.empty());
}

} // namespace

int main() {
    test_single_element_can_be_popped();
    test_every_slot_is_usable();
    test_move_only_values_are_moved_in();
    test_destructor_destroys_remaining_values();
    test_values_cross_threads_in_order();
    return neutron_test::result();
}
