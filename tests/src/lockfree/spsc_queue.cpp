#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <neutron/lockfree.hpp>

namespace {

bool test_single_threaded_fifo() {
    constexpr auto count = 1000;

    neutron::spsc_queue<int> queue(4);

    for (auto i = 0; i != count; ++i) {
        queue.push_back(i);
    }

    for (auto i = 0; i != count; ++i) {
        if (queue.empty()) {
            return false;
        }
        if (queue.front() != i) {
            return false;
        }
        queue.pop_front();
    }

    return queue.empty();
}

// Keeps a few elements in flight, so the producer keeps moving into blocks the
// consumer has left, and `front` crosses block boundaries without `empty`.
bool test_interleaved_block_reuse() {
    constexpr auto rounds = 10'000;
    constexpr auto lag    = 37;

    neutron::spsc_queue<int> queue(1);

    auto expected = 0;
    for (auto i = 0; i != rounds; ++i) {
        queue.push_back(i);
        if (i >= lag) {
            if (queue.front() != expected) {
                return false;
            }
            ++expected;
            queue.pop_front();
        }
    }

    while (expected != rounds) {
        if (queue.front() != expected) {
            return false;
        }
        ++expected;
        queue.pop_front();
    }

    return queue.empty();
}

bool test_non_trivial_type() {
    neutron::spsc_queue<std::string> queue(2);

    queue.push_back("hello");
    queue.push_back(std::string(64, 'x'));

    if (queue.front() != "hello") {
        return false;
    }
    queue.pop_front();

    if (queue.front() != std::string(64, 'x')) {
        return false;
    }
    queue.pop_front();

    return queue.empty();
}

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

bool test_destructor_destroys_values() {
    constexpr auto count = 10;
    {
        neutron::spsc_queue<tracked> queue(4);
        for (auto i = 0; i != count; ++i) {
            queue.push_back(tracked{});
        }
        if (tracked::alive.load(std::memory_order_relaxed) != count) {
            return false;
        }
    }

    return tracked::alive.load(std::memory_order_relaxed) == 0;
}

#if ATOM_ENABLE_EXCEPTIONS

struct bite {
    static inline bool fail = false;

    explicit bite(int) {
        if (fail) {
            throw 0;
        }
    }
};

bool test_exception_safety() {
    neutron::spsc_queue<bite> queue(4);

    queue.push_back(0);

    bite::fail  = true;
    bool caught = false;
    ATOM_TRY { queue.push_back(0); }
    ATOM_CATCH(...) { caught = true; }
    bite::fail = false;

    if (!caught) {
        return false;
    }

    queue.push_back(0);
    queue.pop_front();
    queue.pop_front();

    return queue.empty();
}

struct numbered {
    static inline int fail_at = -1;

    int value;

    explicit numbered(int val) : value(val) {
        if (val == fail_at) {
            throw 0;
        }
    }
};

// Throws on the first element of a new block, then keeps going.
bool test_exception_on_block_switch() {
    constexpr auto count = 64;

    neutron::spsc_queue<numbered> queue(16);

    for (auto i = 0; i != 16; ++i) {
        queue.push_back(i);
    }

    numbered::fail_at = 16;
    bool caught       = false;
    ATOM_TRY { queue.push_back(16); }
    ATOM_CATCH(...) { caught = true; }
    numbered::fail_at = -1;

    if (!caught) {
        return false;
    }

    for (auto i = 16; i != count; ++i) {
        queue.push_back(i);
    }

    for (auto i = 0; i != count; ++i) {
        if (queue.empty() || queue.front().value != i) {
            return false;
        }
        queue.pop_front();
    }

    return queue.empty();
}

#endif // ATOM_ENABLE_EXCEPTIONS

// The producer allocates extra pools on its own thread; another thread owns and
// destroys the queue.
bool test_destroy_after_producer_stops() {
    constexpr auto count = 500;

    auto queue = std::make_unique<neutron::spsc_queue<tracked>>(1);
    {
        std::jthread producer([&queue] {
            for (auto i = 0; i != count; ++i) {
                queue->push_back(tracked{});
            }
        });
    }

    queue.reset();

    return tracked::alive.load(std::memory_order_relaxed) == 0;
}

bool test_concurrent_producer_and_consumer() {
    constexpr auto count = 200'000;
    constexpr auto total = static_cast<std::size_t>(count);

    neutron::spsc_queue<int> queue(8);

    std::jthread producer([&queue] {
        for (auto i = 0; i != count; ++i) {
            queue.push_back(i);
        }
    });

    for (std::size_t expected = 0; expected != total;) {
        if (queue.empty()) {
            std::this_thread::yield();
            continue;
        }
        if (queue.front() != static_cast<int>(expected)) {
            return false;
        }
        ++expected;
        queue.pop_front();
    }

    producer.join();

    return queue.empty();
}

// Heap-owning values catch slots that are reused before they are destroyed.
bool test_concurrent_non_trivial_type() {
    constexpr auto count = 100'000;

    neutron::spsc_queue<std::string> queue(1);

    std::jthread producer([&queue] {
        for (auto i = 0; i != count; ++i) {
            queue.push_back(std::to_string(i) + std::string(24, 'x'));
        }
    });

    for (auto expected = 0; expected != count;) {
        if (queue.empty()) {
            continue;
        }
        if (queue.front() != std::to_string(expected) + std::string(24, 'x')) {
            return false;
        }
        ++expected;
        queue.pop_front();
    }

    producer.join();

    return queue.empty();
}

} // namespace

int main() {
    if (!test_single_threaded_fifo()) {
        return 1;
    }
    if (!test_interleaved_block_reuse()) {
        return 2;
    }
    if (!test_non_trivial_type()) {
        return 3;
    }
    if (!test_destructor_destroys_values()) {
        return 4;
    }
    if (!test_destroy_after_producer_stops()) {
        return 5;
    }
#if ATOM_ENABLE_EXCEPTIONS
    if (!test_exception_safety()) {
        return 6;
    }
    if (!test_exception_on_block_switch()) {
        return 8;
    }
#endif
    if (!test_concurrent_producer_and_consumer()) {
        return 7;
    }
    if (!test_concurrent_non_trivial_type()) {
        return 9;
    }
    return 0;
}
