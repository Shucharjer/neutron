#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <neutron/lockfree.hpp>
#include "require.hpp"

namespace {

struct node {
    std::atomic<node*> next{ nullptr };
    std::size_t value = 0;
};

/// A node which can be linked into two queues at the same time.
struct dual_node {
    std::atomic<dual_node*> first{ nullptr };
    std::atomic<dual_node*> second{ nullptr };
    std::string payload;
};

using queue_t = neutron::intrusive_spsc_queue<&node::next>;

/// Heap-owning payload of the `index`-th dual node.
std::string payload_of(std::size_t index) {
    constexpr std::size_t length = 32;
    return std::string(length, static_cast<char>('a' + index));
}

// The queue is pinned in place: neither copyable nor movable.
static_assert(std::is_nothrow_default_constructible_v<queue_t>);
static_assert(std::is_nothrow_destructible_v<queue_t>);
static_assert(!std::is_copy_constructible_v<queue_t>);
static_assert(!std::is_move_constructible_v<queue_t>);
static_assert(!std::is_copy_assignable_v<queue_t>);
static_assert(!std::is_move_assignable_v<queue_t>);

// Every operation is noexcept; push_back reports whether the queue was empty.
static_assert(requires(queue_t& queue, const queue_t& cqueue, node* ptr) {
    { queue.push_back(ptr) } noexcept -> std::same_as<bool>;
    { queue.pop_front() } noexcept -> std::same_as<node*>;
    { cqueue.front() } noexcept -> std::same_as<node*>;
    { cqueue.empty() } noexcept -> std::same_as<bool>;
});

void test_empty_queue_has_no_front() {
    queue_t queue;

    require(queue.empty());
    require(queue.front() == nullptr);
    require(queue.pop_front() == nullptr);
    require(queue.pop_front() == nullptr);
    require(queue.empty());
}

void test_single_element_round_trip() {
    constexpr std::size_t value = 42;
    queue_t queue;
    node item{ .value = value };

    require(queue.push_back(&item));
    require_false(queue.empty());
    require(queue.front() == &item);
    require(queue.pop_front() == &item);

    require(queue.empty());
    require(queue.front() == nullptr);
    require(queue.pop_front() == nullptr);
}

void test_elements_pop_in_fifo_order() {
    constexpr std::size_t count = 16;
    queue_t queue;
    std::array<node, count> nodes{};

    for (std::size_t i = 0; i != count; ++i) {
        nodes[i].value = i;
        // Only the first push sees an empty queue.
        require(queue.push_back(&nodes[i]) == (i == 0));
    }

    for (std::size_t i = 0; i != count; ++i) {
        node* const front = queue.front();
        require_or_return(front == &nodes[i], );
        require(queue.pop_front() == front);
        require(front->value == i);
    }
    require(queue.empty());
    require(queue.pop_front() == nullptr);
}

void test_queue_is_reusable_after_drain() {
    constexpr std::size_t rounds = 8;
    constexpr std::size_t count  = 5;
    queue_t queue;
    std::array<node, count> nodes{};

    // Draining relinks the stub, so every round starts from an empty queue.
    for (std::size_t r = 0; r != rounds; ++r) {
        const std::size_t size = (r % count) + 1;
        for (std::size_t i = 0; i != size; ++i) {
            nodes[i].value = (r * count) + i;
            require(queue.push_back(&nodes[i]) == (i == 0));
        }

        for (std::size_t i = 0; i != size; ++i) {
            node* const ptr = queue.pop_front();
            require_or_return(ptr == &nodes[i], );
            require(ptr->value == (r * count) + i);
        }
        require(queue.empty());
        require(queue.pop_front() == nullptr);
    }
}

void test_interleaved_push_pop_keeps_order() {
    constexpr std::size_t count = 64;
    queue_t queue;
    std::array<node, count> nodes{};
    std::size_t pushed = 0;
    std::size_t popped = 0;

    // Keep one or two elements in flight, so that the stub is relinked while
    // the queue is not empty.
    while (popped != count) {
        if (pushed != count) {
            nodes[pushed].value  = pushed;
            const bool was_empty = queue.push_back(&nodes[pushed]);
            require(was_empty == (pushed == popped));
            ++pushed;
        }
        if (pushed % 3 != 1 || pushed == count) {
            node* const ptr = queue.pop_front();
            require_or_return(ptr != nullptr, );
            require_or_return(ptr == &nodes[popped], );
            require(ptr->value == popped);
            ++popped;
        }
    }

    require(queue.empty());
}

void test_popped_node_can_be_pushed_again() {
    constexpr int rounds = 4;
    queue_t queue;
    node lhs{ .value = 1 };
    node rhs{ .value = 2 };

    for (int i = 0; i != rounds; ++i) {
        queue.push_back(&lhs);
        queue.push_back(&rhs);
        require(queue.pop_front() == &lhs);

        // Push a popped node while another one is still queued.
        queue.push_back(&lhs);

        require(queue.pop_front() == &rhs);
        require(queue.pop_front() == &lhs);
        require(queue.empty());
    }
}

void test_node_can_be_in_two_queues() {
    constexpr std::size_t count = 4;
    neutron::intrusive_spsc_queue<&dual_node::first> first;
    neutron::intrusive_spsc_queue<&dual_node::second> second;
    std::array<dual_node, count> nodes{};

    for (std::size_t i = 0; i != count; ++i) {
        nodes[i].payload = payload_of(i);
        first.push_back(&nodes[i]);
        second.push_back(&nodes[count - 1 - i]);
    }

    for (std::size_t i = 0; i != count; ++i) {
        dual_node* const lhs = first.pop_front();
        dual_node* const rhs = second.pop_front();
        require_or_return(lhs == &nodes[i], );
        require(rhs == &nodes[count - 1 - i]);
        require(lhs->payload == payload_of(i));
    }
    require(first.empty());
    require(second.empty());
}

void test_values_cross_threads_in_order() {
    constexpr std::size_t count = 1 << 17;
    queue_t queue;
    std::vector<node> nodes(count);
    std::vector<unsigned char> seen(count, 0);
    std::atomic<bool> in_order = true;

    std::thread consumer([&] {
        for (std::size_t expected = 0; expected != count;) {
            node* const front = queue.front();
            node* const ptr   = queue.pop_front();
            if (ptr == nullptr) {
                continue;
            }
            // Only this thread pops, so a visible front must be popped next.
            if ((front != nullptr && front != ptr) || ptr->value != expected) {
                in_order.store(false, std::memory_order_relaxed);
            }
            ++seen[ptr->value % count];
            ++expected;
        }
    });
    std::thread producer([&] {
        for (std::size_t i = 0; i != count; ++i) {
            nodes[i].value = i;
            queue.push_back(&nodes[i]);
        }
    });
    producer.join();
    consumer.join();

    require(in_order.load(std::memory_order_relaxed));
    require(queue.empty());
    for (const auto times : seen) {
        require(times == 1);
    }
}

void test_nodes_recycle_between_two_threads() {
    constexpr std::size_t count     = 1 << 16;
    constexpr std::size_t pool_size = 4;

    // Nodes go forward with a value and come back through `backward`, so both
    // queues keep hitting the empty and single element states.
    queue_t forward;
    queue_t backward;
    std::array<node, pool_size> pool{};
    for (auto& item : pool) {
        backward.push_back(&item);
    }
    std::atomic<bool> in_order = true;
    std::size_t received       = 0;

    std::thread consumer([&] {
        for (std::size_t expected = 0; expected != count;) {
            node* const ptr = forward.pop_front();
            if (ptr == nullptr) {
                continue;
            }
            if (ptr->value != expected) {
                in_order.store(false, std::memory_order_relaxed);
            }
            ++expected;
            ++received;
            backward.push_back(ptr);
        }
    });
    std::thread producer([&] {
        for (std::size_t i = 0; i != count;) {
            node* const ptr = backward.pop_front();
            if (ptr == nullptr) {
                continue;
            }
            ptr->value = i;
            forward.push_back(ptr);
            ++i;
        }
    });
    producer.join();
    consumer.join();

    require(in_order.load(std::memory_order_relaxed));
    require(received == count);
    require(forward.empty());

    // Every node must be back in the pool exactly once.
    std::array<bool, pool_size> returned{};
    for (std::size_t i = 0; i != pool_size; ++i) {
        node* const ptr = backward.pop_front();
        require_or_return(ptr != nullptr, );
        require_or_return(ptr >= pool.data(), );
        require_or_return(ptr < pool.data() + pool_size, );
        auto& flag = returned[static_cast<std::size_t>(ptr - pool.data())];
        require_false(flag);
        flag = true;
    }
    require(backward.empty());
}

} // namespace

int main() {
    test_empty_queue_has_no_front();
    test_single_element_round_trip();
    test_elements_pop_in_fifo_order();
    test_queue_is_reusable_after_drain();
    test_interleaved_push_pop_keeps_order();
    test_popped_node_can_be_pushed_again();
    test_node_can_be_in_two_queues();
    test_values_cross_threads_in_order();
    test_nodes_recycle_between_two_threads();
    return neutron_test::result();
}
