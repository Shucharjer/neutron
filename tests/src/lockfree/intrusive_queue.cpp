#include <concepts>
#include <type_traits>
#include <utility>
#include <neutron/lockfree.hpp>
#include "require.hpp"

namespace {

struct node {
    node* next = nullptr;
    int value  = 0;
};

struct other_node {
    other_node* link = nullptr;
};

using queue_t = neutron::intrusive_queue<&node::next>;

// The queue is a move-only handle that never throws.
static_assert(std::is_nothrow_default_constructible_v<queue_t>);
static_assert(std::is_nothrow_move_constructible_v<queue_t>);
static_assert(std::is_nothrow_destructible_v<queue_t>);
static_assert(!std::is_copy_constructible_v<queue_t>);
static_assert(!std::is_copy_assignable_v<queue_t>);
static_assert(std::is_nothrow_move_assignable_v<queue_t>);

// Any pointer member can serve as the link.
static_assert(std::is_default_constructible_v<
              neutron::intrusive_queue<&other_node::link>>);

static_assert(requires(const queue_t& queue) {
    { queue.empty() } noexcept -> std::same_as<bool>;
});

/// True if a default constructed queue is empty in constant evaluation.
constexpr bool default_constructed_is_empty() {
    const queue_t queue;
    return queue.empty();
}
static_assert(default_constructed_is_empty());

/// True if both sides of a move construction are empty in constant
/// evaluation.
constexpr bool move_construction_keeps_both_empty() {
    queue_t source;
    queue_t target(std::move(source));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    return source.empty() && target.empty();
}
static_assert(move_construction_keeps_both_empty());

/// True if both sides of a move assignment are empty in constant evaluation.
constexpr bool move_assignment_keeps_both_empty() {
    queue_t source;
    queue_t target;
    target = std::move(source);
    return source.empty() && target.empty(); // NOLINT(bugprone-use-after-move)
}
static_assert(move_assignment_keeps_both_empty());

void test_move_construction_keeps_both_empty_at_runtime() {
    queue_t queue;

    queue_t moved(std::move(queue));

    require(queue.empty()); // NOLINT(bugprone-use-after-move)
    require(moved.empty());
}

} // namespace

int main() {
    test_move_construction_keeps_both_empty_at_runtime();
    return neutron_test::result();
}
