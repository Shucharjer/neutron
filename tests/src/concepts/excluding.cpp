#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Arbitrary class type.
struct foo {};

// satisfied by: the first type is none of the others
static_assert(excluding<int>);
static_assert(excluding<int, long>);
static_assert(excluding<int, long, float, foo>);

// rejected: the first type appears among the others
static_assert(!excluding<int, int>);
static_assert(!excluding<int, long, int>);
static_assert(!excluding<foo, int, foo>);

// rejected: cvref of the first argument is removed
static_assert(!excluding<const int, int>);
static_assert(!excluding<int&, int>);
static_assert(!excluding<const volatile int&&, int>);
static_assert(!excluding<const foo&, foo>);

// satisfied by: the excluded types are compared exactly
static_assert(excluding<int, const int>);
static_assert(excluding<int, int&>);
static_assert(excluding<const int&, const int&>);

// pointers and arrays are not decayed
static_assert(excluding<int*, int>);
static_assert(excluding<int[3], int*>);
static_assert(!excluding<int (&)[3], int[3]>);
static_assert(excluding<void, int>);
static_assert(!excluding<const void, void>);

} // namespace

int main() { return neutron_test::result(); }
