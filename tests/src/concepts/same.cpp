#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Arbitrary class type.
struct foo {};

/// Aliases denote the same type.
using int_alias = int;

// satisfied by: all types identical, including a single type
static_assert(same<int>);
static_assert(same<void>);
static_assert(same<int, int>);
static_assert(same<int, int, int, int_alias>);
static_assert(same<foo, foo>);
static_assert(same<const int&, const int&>);

// rejected: any type differs, wherever it is
static_assert(!same<int, long>);
static_assert(!same<int, int, long>);
static_assert(!same<long, int, int>);
static_assert(!same<foo, int>);

// rejected: no cvref or decay adjustments
static_assert(!same<int, const int>);
static_assert(!same<int, int&>);
static_assert(!same<int&, int&&>);
static_assert(!same<int*, int[3]>);
static_assert(!same<void, const void>);

} // namespace

int main() { return neutron_test::result(); }
