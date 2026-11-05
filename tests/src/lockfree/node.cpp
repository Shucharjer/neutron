#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <neutron/lockfree.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

template <typename T>
using node = internal::_lockfree_node<T>;

struct alignas(64) over_aligned {
    std::uint64_t value;
    char tail;
};

/// Counts live instances to catch leaks and double destruction.
struct counted {
    static inline int alive = 0;

    explicit counted(int val) : value(val) { ++alive; }
    counted(const counted&)            = delete;
    counted& operator=(const counted&) = delete;
    ~counted() { --alive; }

    int value;
};

struct throwing_ctor {
    throwing_ctor() noexcept(false) {}
};

// The payload is stored in suitably aligned raw bytes.
static_assert(alignof(node<over_aligned>) >= alignof(over_aligned));
static_assert(sizeof(node<over_aligned>) >= sizeof(over_aligned));
static_assert(offsetof(node<over_aligned>, bytes) % alignof(over_aligned) == 0);
static_assert(alignof(node<char>) >= alignof(std::atomic<node<char>*>));

// construct() is noexcept exactly when the payload constructor is.
static_assert(noexcept(std::declval<node<int>&>().construct(1)));
static_assert(!noexcept(std::declval<node<throwing_ctor>&>().construct()));

static_assert(std::is_same_v<
              decltype(std::declval<const node<int>&>().value()), const int*>);

void test_construct_and_destroy_string() {
    // Heap-allocated so that ASan watches the payload.
    auto holder = std::make_unique<node<std::string>>();

    const std::string text(100, 'q');
    std::string* const payload = holder->construct(text);
    require(payload == holder->value());
    require(static_cast<void*>(payload) == static_cast<void*>(holder->bytes));
    require(*payload == text);

    holder->value()->append("!");
    const auto& const_node = *holder;
    require(const_node.value()->size() == 101);
    require(const_node.value()->back() == '!');
    holder->destroy();
}

void test_storage_is_reusable() {
    auto holder = std::make_unique<node<std::string>>();
    holder->construct("first");
    holder->destroy();

    holder->construct(3, 'z');
    require(*holder->value() == "zzz");
    holder->destroy();
}

void test_over_aligned_payload() {
    auto holder   = std::make_unique<node<over_aligned>>();
    auto* payload = holder->construct(over_aligned{ 42, 'x' });

    const auto address = reinterpret_cast<std::uintptr_t>(payload);
    require(address % alignof(over_aligned) == 0);
    require(holder->value()->value == 42);
    require(payload->tail == 'x');
    holder->destroy();
}

void test_destroy_ends_lifetime() {
    node<counted> storage{};
    storage.construct(7);
    require(counted::alive == 1);
    require(storage.value()->value == 7);

    storage.destroy();
    require(counted::alive == 0);
}

void test_nodes_link_through_next() {
    node<int> first{};
    node<int> second{};
    first.next.store(&second, std::memory_order_relaxed);
    second.next.store(nullptr, std::memory_order_relaxed);
    first.construct(1);
    second.construct(2);

    node<int>* const next = first.next.load(std::memory_order_relaxed);
    require(next == &second);
    require(*first.value() == 1);
    require(*next->value() == 2);
    require(next->next.load(std::memory_order_relaxed) == nullptr);
}

} // namespace

int main() {
    test_construct_and_destroy_string();
    test_storage_is_reusable();
    test_over_aligned_payload();
    test_destroy_ends_lifetime();
    test_nodes_link_through_next();
    return neutron_test::result();
}
