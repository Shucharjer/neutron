#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Arbitrary class type.
struct foo {};

// satisfied by: the first type appears among the others
static_assert(one_of<int, int>);
static_assert(one_of<int, long, int>);
static_assert(one_of<int, float, double, int>);
static_assert(one_of<void, void, bool>);
static_assert(one_of<foo, int, foo>);
static_assert(one_of<const int&, int, const int&>);

// rejected: no match, including an empty list
static_assert(!one_of<int>);
static_assert(!one_of<int, long>);
static_assert(!one_of<int, long, unsigned>);
static_assert(!one_of<foo, int, long>);

// rejected: no cvref or decay adjustments
static_assert(!one_of<const int, int>);
static_assert(!one_of<int&, int>);
static_assert(!one_of<int, int&, const int, int&&>);
static_assert(!one_of<int[3], int*>);
static_assert(!one_of<void, const void>);

} // namespace

int main() { return neutron_test::result(); }
