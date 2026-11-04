#include <type_traits>
#include <utility>
#include <neutron/lockfree.hpp>

namespace {

struct node {
    node* next = nullptr;
    int value  = 0;
};

struct other_node {
    other_node* link = nullptr;
};

using queue_t = neutron::intrusive_queue<&node::next>;

static_assert(std::is_nothrow_default_constructible_v<queue_t>);
static_assert(std::is_nothrow_move_constructible_v<queue_t>);
static_assert(std::is_nothrow_destructible_v<queue_t>);
static_assert(!std::is_copy_constructible_v<queue_t>);
static_assert(!std::is_copy_assignable_v<queue_t>);
static_assert(std::is_nothrow_move_assignable_v<queue_t>);
static_assert(std::is_default_constructible_v<
              neutron::intrusive_queue<&other_node::link>>);
static_assert(requires(const queue_t& queue) {
    { queue.empty() } noexcept -> std::same_as<bool>;
});

constexpr bool test_default_constructed_is_empty() {
    const queue_t queue;
    return queue.empty();
}

constexpr bool test_move_construction_keeps_both_empty() {
    queue_t source;
    queue_t target(std::move(source));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    return source.empty() && target.empty();
}

static_assert(test_default_constructed_is_empty());
static_assert(test_move_construction_keeps_both_empty());

constexpr bool test_move_assignment_keeps_both_empty() {
    queue_t source;
    queue_t target;
    target = std::move(source);
    return source.empty() && target.empty(); // NOLINT(bugprone-use-after-move)
}
static_assert(test_move_assignment_keeps_both_empty());

bool test_runtime_empty() {
    queue_t queue;
    queue_t moved(std::move(queue));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    return queue.empty() && moved.empty();
}

} // namespace

int main() {
    if (!test_runtime_empty()) {
        return 1;
    }
    return 0;
}
