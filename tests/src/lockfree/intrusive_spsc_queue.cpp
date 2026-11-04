#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <neutron/lockfree.hpp>

namespace {

struct node {
    std::atomic<node*> next{ nullptr };
    std::size_t value = 0;
};

/// @brief A node which can be linked into two queues at the same time.
struct dual_node {
    std::atomic<dual_node*> first{ nullptr };
    std::atomic<dual_node*> second{ nullptr };
    std::string payload;
};

using queue_t = neutron::intrusive_spsc_queue<&node::next>;

static_assert(std::is_nothrow_default_constructible_v<queue_t>);
static_assert(std::is_nothrow_destructible_v<queue_t>);
static_assert(!std::is_copy_constructible_v<queue_t>);
static_assert(!std::is_move_constructible_v<queue_t>);
static_assert(!std::is_copy_assignable_v<queue_t>);
static_assert(!std::is_move_assignable_v<queue_t>);
static_assert(requires(queue_t& queue, const queue_t& cqueue, node* ptr) {
    { queue.push_back(ptr) } noexcept -> std::same_as<bool>;
    { queue.pop_front() } noexcept -> std::same_as<node*>;
    { cqueue.front() } noexcept -> std::same_as<node*>;
    { cqueue.empty() } noexcept -> std::same_as<bool>;
});

bool test_empty_queue() {
    queue_t queue;
    return queue.empty() && queue.front() == nullptr &&
           queue.pop_front() == nullptr && queue.pop_front() == nullptr &&
           queue.empty();
}

bool test_single_element() {
    queue_t queue;
    node item{ .value = 42 };
    if (!queue.push_back(&item)) {
        return false;
    }
    if (queue.empty() || queue.front() != &item) {
        return false;
    }
    if (queue.pop_front() != &item) {
        return false;
    }
    return queue.empty() && queue.front() == nullptr &&
           queue.pop_front() == nullptr;
}

bool test_fifo_order() {
    constexpr std::size_t count = 16;

    queue_t queue;
    std::array<node, count> nodes{};
    for (std::size_t i = 0; i != count; ++i) {
        nodes[i].value = i;
        // Only the first push sees an empty queue.
        if (queue.push_back(&nodes[i]) != (i == 0)) {
            return false;
        }
    }

    for (std::size_t i = 0; i != count; ++i) {
        node* const front = queue.front();
        if (front != &nodes[i] || queue.pop_front() != front) {
            return false;
        }
        if (front->value != i) {
            return false;
        }
    }
    return queue.empty() && queue.pop_front() == nullptr;
}

bool test_reuse_after_drain() {
    constexpr std::size_t round = 8;
    constexpr std::size_t count = 5;

    queue_t queue;
    std::array<node, count> nodes{};
    // Draining relinks the stub, so every round starts from an empty queue.
    for (std::size_t r = 0; r != round; ++r) {
        const std::size_t size = (r % count) + 1;
        for (std::size_t i = 0; i != size; ++i) {
            nodes[i].value = (r * count) + i;
            if (queue.push_back(&nodes[i]) != (i == 0)) {
                return false;
            }
        }
        for (std::size_t i = 0; i != size; ++i) {
            node* const ptr = queue.pop_front();
            if (ptr != &nodes[i] || ptr->value != (r * count) + i) {
                return false;
            }
        }
        if (!queue.empty() || queue.pop_front() != nullptr) {
            return false;
        }
    }
    return true;
}

bool test_interleaved_push_pop() {
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
            if (was_empty != (pushed == popped)) {
                return false;
            }
            ++pushed;
        }
        if (pushed % 3 != 1 || pushed == count) {
            node* const ptr = queue.pop_front();
            if (ptr == nullptr || ptr != &nodes[popped] ||
                ptr->value != popped) {
                return false;
            }
            ++popped;
        }
    }
    return queue.empty();
}

bool test_node_can_be_pushed_again() {
    queue_t queue;
    node lhs{ .value = 1 };
    node rhs{ .value = 2 };
    for (int i = 0; i != 4; ++i) {
        queue.push_back(&lhs);
        queue.push_back(&rhs);
        if (queue.pop_front() != &lhs) {
            return false;
        }
        // Push a popped node while another one is still queued.
        queue.push_back(&lhs);
        if (queue.pop_front() != &rhs || queue.pop_front() != &lhs) {
            return false;
        }
        if (!queue.empty()) {
            return false;
        }
    }
    return true;
}

bool test_node_in_two_queues() {
    constexpr std::size_t count = 4;

    neutron::intrusive_spsc_queue<&dual_node::first> first;
    neutron::intrusive_spsc_queue<&dual_node::second> second;
    std::array<dual_node, count> nodes{};
    for (std::size_t i = 0; i != count; ++i) {
        nodes[i].payload = std::string(32, static_cast<char>('a' + i));
        first.push_back(&nodes[i]);
        second.push_back(&nodes[count - 1 - i]);
    }

    for (std::size_t i = 0; i != count; ++i) {
        dual_node* const lhs = first.pop_front();
        dual_node* const rhs = second.pop_front();
        if (lhs != &nodes[i] || rhs != &nodes[count - 1 - i]) {
            return false;
        }
        if (lhs->payload != std::string(32, static_cast<char>('a' + i))) {
            return false;
        }
    }
    return first.empty() && second.empty();
}

bool test_values_cross_threads_in_order() {
    constexpr std::size_t count = 1 << 17;

    queue_t queue;
    std::vector<node> nodes(count);
    std::vector<unsigned char> seen(count, 0);
    std::atomic<bool> valid = true;

    std::thread consumer([&] {
        for (std::size_t expected = 0; expected != count;) {
            node* const front = queue.front();
            node* const ptr   = queue.pop_front();
            if (ptr == nullptr) {
                continue;
            }
            // Only this thread pops, so a visible front must be popped next.
            if ((front != nullptr && front != ptr) || ptr->value != expected) {
                valid.store(false, std::memory_order_relaxed);
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

    if (!valid.load(std::memory_order_relaxed) || !queue.empty()) {
        return false;
    }
    for (const auto times : seen) {
        if (times != 1) {
            return false;
        }
    }
    return true;
}

bool test_nodes_recycled_across_threads() {
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

    std::atomic<bool> valid = true;
    std::size_t received    = 0;

    std::thread consumer([&] {
        for (std::size_t expected = 0; expected != count;) {
            node* const ptr = forward.pop_front();
            if (ptr == nullptr) {
                continue;
            }
            if (ptr->value != expected) {
                valid.store(false, std::memory_order_relaxed);
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

    if (!valid.load(std::memory_order_relaxed) || received != count) {
        return false;
    }
    if (!forward.empty()) {
        return false;
    }
    // Every node must be back in the pool exactly once.
    std::array<bool, pool_size> returned{};
    for (std::size_t i = 0; i != pool_size; ++i) {
        node* const ptr = backward.pop_front();
        if (ptr == nullptr || ptr < pool.data() ||
            ptr >= pool.data() + pool_size) {
            return false;
        }
        auto& flag = returned[static_cast<std::size_t>(ptr - pool.data())];
        if (flag) {
            return false;
        }
        flag = true;
    }
    return backward.empty();
}

} // namespace

int main() {
    if (!test_empty_queue()) {
        return 1;
    }
    if (!test_single_element()) {
        return 2;
    }
    if (!test_fifo_order()) {
        return 3;
    }
    if (!test_reuse_after_drain()) {
        return 4;
    }
    if (!test_interleaved_push_pop()) {
        return 5;
    }
    if (!test_node_can_be_pushed_again()) {
        return 6;
    }
    if (!test_node_in_two_queues()) {
        return 7;
    }
    if (!test_values_cross_threads_in_order()) {
        return 8;
    }
    if (!test_nodes_recycled_across_threads()) {
        return 9;
    }
    return 0;
}
