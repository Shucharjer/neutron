#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <neutron/lockfree.hpp>
#include "require.hpp"

namespace {

/// Counts live instances to catch leaks and double destruction.
struct tracked {
    static inline std::atomic<int> alive{ 0 };

    tracked() noexcept { alive.fetch_add(1, std::memory_order_relaxed); }
    tracked(const tracked&) noexcept {
        alive.fetch_add(1, std::memory_order_relaxed);
    }
    tracked(tracked&&) noexcept {
        alive.fetch_add(1, std::memory_order_relaxed);
    }
    tracked& operator=(const tracked&) = default;
    tracked& operator=(tracked&&)      = default;

    ~tracked() { alive.fetch_sub(1, std::memory_order_relaxed); }
};

/// Heap-owning string for the `index`-th value.
std::string long_string_of(int index) {
    constexpr std::size_t padding = 24;
    return std::to_string(index) + std::string(padding, 'x');
}

void test_single_threaded_fifo() {
    constexpr auto count            = 1000;
    constexpr auto initial_capacity = 4;
    neutron::spsc_queue<int> queue(initial_capacity);

    for (auto i = 0; i != count; ++i) {
        queue.push_back(i);
    }

    for (auto i = 0; i != count; ++i) {
        require_or_return(!queue.empty(), );
        require(queue.front() == i);
        queue.pop_front();
    }
    require(queue.empty());
}

// Keeps a few elements in flight, so the producer keeps moving into blocks the
// consumer has left, and `front` crosses block boundaries without `empty`.
void test_interleaved_push_pop_reuses_blocks() {
    constexpr auto rounds = 10'000;
    constexpr auto lag    = 37;
    neutron::spsc_queue<int> queue(1);
    auto expected = 0;

    for (auto i = 0; i != rounds; ++i) {
        queue.push_back(i);
        if (i >= lag) {
            require(queue.front() == expected);
            ++expected;
            queue.pop_front();
        }
    }
    while (expected != rounds) {
        require(queue.front() == expected);
        ++expected;
        queue.pop_front();
    }

    require(queue.empty());
}

void test_non_trivial_values_keep_their_content() {
    constexpr std::size_t length = 64;
    neutron::spsc_queue<std::string> queue(2);

    queue.push_back("hello");
    queue.push_back(std::string(length, 'x'));

    require(queue.front() == "hello");
    queue.pop_front();
    require(queue.front() == std::string(length, 'x'));
    queue.pop_front();
    require(queue.empty());
}

void test_destructor_destroys_remaining_values() {
    constexpr auto count            = 10;
    constexpr auto initial_capacity = 4;
    {
        neutron::spsc_queue<tracked> queue(initial_capacity);
        for (auto i = 0; i != count; ++i) {
            queue.push_back(tracked{});
        }

        require(tracked::alive.load(std::memory_order_relaxed) == count);
    }

    require(tracked::alive.load(std::memory_order_relaxed) == 0);
}

// The producer allocates extra pools on its own thread; another thread owns and
// destroys the queue.
void test_destroy_after_producer_stops() {
    constexpr auto count = 500;
    auto queue           = std::make_unique<neutron::spsc_queue<tracked>>(1);

    {
        std::jthread producer([&queue] {
            for (auto i = 0; i != count; ++i) {
                queue->push_back(tracked{});
            }
        });
    } // Joins the producer.
    queue.reset();

    require(tracked::alive.load(std::memory_order_relaxed) == 0);
}

#if ATOM_ENABLE_EXCEPTIONS

/// Throws from its constructor while `fail` is set.
struct bite {
    static inline bool fail = false;

    explicit bite(int) {
        if (fail) {
            throw 0;
        }
    }
};

void test_throwing_push_leaves_queue_usable() {
    constexpr auto initial_capacity = 4;
    neutron::spsc_queue<bite> queue(initial_capacity);
    queue.push_back(0);

    bite::fail  = true;
    bool caught = false;
    ATOM_TRY { queue.push_back(0); }
    ATOM_CATCH(...) { caught = true; }
    bite::fail = false;

    require(caught);

    queue.push_back(0);
    queue.pop_front();
    queue.pop_front();

    require(queue.empty());
}

/// Throws from its constructor when built from `fail_at`.
struct numbered {
    static inline int fail_at = -1;

    int value;

    explicit numbered(int val) : value(val) {
        if (val == fail_at) {
            throw 0;
        }
    }
};

// Throws on the first element of a new block, then keeps going. A request of
// 16 is the minimum, so the first block holds exactly that many values.
void test_throwing_push_on_block_switch_keeps_order() {
    constexpr auto count                = 64;
    constexpr auto first_block_capacity = 16;
    neutron::spsc_queue<numbered> queue(first_block_capacity);
    for (auto i = 0; i != first_block_capacity; ++i) {
        queue.push_back(i);
    }

    numbered::fail_at = first_block_capacity;
    bool caught       = false;
    ATOM_TRY { queue.push_back(first_block_capacity); }
    ATOM_CATCH(...) { caught = true; }
    numbered::fail_at = -1;

    require(caught);

    for (auto i = first_block_capacity; i != count; ++i) {
        queue.push_back(i);
    }

    for (auto i = 0; i != count; ++i) {
        require_or_return(!queue.empty(), );
        require(queue.front().value == i);
        queue.pop_front();
    }
    require(queue.empty());
}

#endif // ATOM_ENABLE_EXCEPTIONS

void test_concurrent_producer_and_consumer_keep_order() {
    constexpr auto count            = 200'000;
    constexpr auto total            = static_cast<std::size_t>(count);
    constexpr auto initial_capacity = 8;
    neutron::spsc_queue<int> queue(initial_capacity);
    bool in_order = true;

    std::jthread producer([&queue] {
        for (auto i = 0; i != count; ++i) {
            queue.push_back(i);
        }
    });
    // Keep draining on mismatch; results are checked after the join.
    for (std::size_t expected = 0; expected != total;) {
        if (queue.empty()) {
            std::this_thread::yield();
            continue;
        }
        if (queue.front() != static_cast<int>(expected)) {
            in_order = false;
        }
        ++expected;
        queue.pop_front();
    }
    producer.join();

    require(in_order);
    require(queue.empty());
}

// Heap-owning values catch slots that are reused before they are destroyed.
void test_concurrent_non_trivial_values_keep_order() {
    constexpr auto count = 100'000;
    neutron::spsc_queue<std::string> queue(1);
    bool in_order = true;

    std::jthread producer([&queue] {
        for (auto i = 0; i != count; ++i) {
            queue.push_back(long_string_of(i));
        }
    });
    // Keep draining on mismatch; results are checked after the join.
    for (auto expected = 0; expected != count;) {
        if (queue.empty()) {
            continue;
        }
        if (queue.front() != long_string_of(expected)) {
            in_order = false;
        }
        ++expected;
        queue.pop_front();
    }
    producer.join();

    require(in_order);
    require(queue.empty());
}

} // namespace

int main() {
    test_single_threaded_fifo();
    test_interleaved_push_pop_reuses_blocks();
    test_non_trivial_values_keep_their_content();
    test_destructor_destroys_remaining_values();
    test_destroy_after_producer_stops();
#if ATOM_ENABLE_EXCEPTIONS
    test_throwing_push_leaves_queue_usable();
    test_throwing_push_on_block_switch_keeps_order();
#endif
    test_concurrent_producer_and_consumer_keep_order();
    test_concurrent_non_trivial_values_keep_order();
    return neutron_test::result();
}
