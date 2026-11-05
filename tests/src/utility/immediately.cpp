#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Tag dispatch between a deferred and an immediate overload.
struct task {
    int runs = 0;

    constexpr void schedule() noexcept {}

    constexpr void schedule(immediately_t) noexcept { ++runs; }
};

// A cheap, empty tag type with a const tag object.
static_assert(std::is_empty_v<immediately_t>);
static_assert(std::is_trivially_copyable_v<immediately_t>);
static_assert(std::is_nothrow_default_constructible_v<immediately_t>);
static_assert(std::is_same_v<decltype(immediately), const immediately_t>);

// The tag object is usable in constant expressions.
constexpr immediately_t copy = immediately;

constexpr bool dispatches_in_constexpr() {
    task value;

    value.schedule();
    value.schedule(immediately);
    value.schedule(copy);

    return value.runs == 2;
}
static_assert(dispatches_in_constexpr());

void test_only_tagged_calls_run_immediately() {
    task value;

    value.schedule(immediately);
    value.schedule();
    value.schedule(immediately_t{});

    require(value.runs == 2);
}

} // namespace

int main() {
    test_only_tagged_calls_run_immediately();
    return neutron_test::result();
}
