#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Arbitrary class type.
struct foo {};

// value: satisfied by non-reference types
static_assert(value<int>);
static_assert(value<const int>);
static_assert(value<volatile foo>);
static_assert(value<int*>);
static_assert(value<int[3]>);
static_assert(value<void>);
static_assert(value<void()>);

// value: rejected: references
static_assert(!value<int&>);
static_assert(!value<const int&>);
static_assert(!value<int&&>);
static_assert(!value<int (&)[3]>);
static_assert(!value<void (&)()>);

// lvalue_ref: satisfied by
static_assert(lvalue_ref<int&>);
static_assert(lvalue_ref<const int&>);
static_assert(lvalue_ref<foo&>);
static_assert(lvalue_ref<void (&)()>);

// lvalue_ref: rejected
static_assert(!lvalue_ref<int>);
static_assert(!lvalue_ref<int&&>);
static_assert(!lvalue_ref<const int&&>);
static_assert(!lvalue_ref<int*>);

// clvalue_ref: satisfied by lvalue references to const
static_assert(clvalue_ref<const int&>);
static_assert(clvalue_ref<const volatile int&>);
static_assert(clvalue_ref<const foo&>);
static_assert(clvalue_ref<const int (&)[3]>);
static_assert(clvalue_ref<int* const&>);

// clvalue_ref: rejected
static_assert(!clvalue_ref<int&>);
static_assert(!clvalue_ref<volatile int&>);
static_assert(!clvalue_ref<const int>);
static_assert(!clvalue_ref<const int&&>);
// the pointer itself is not const
static_assert(!clvalue_ref<const int*&>);

// rvalue_ref: satisfied by
static_assert(rvalue_ref<int&&>);
static_assert(rvalue_ref<const int&&>);
static_assert(rvalue_ref<foo&&>);

// rvalue_ref: rejected
static_assert(!rvalue_ref<int>);
static_assert(!rvalue_ref<int&>);
static_assert(!rvalue_ref<const int&>);

/// Number of the disjoint categories `value`, `lvalue_ref` and `rvalue_ref`
/// that `Ty` belongs to.
template <typename Ty>
constexpr int category_count =
    int{ value<Ty> } + int{ lvalue_ref<Ty> } + int{ rvalue_ref<Ty> };

// every type is in exactly one category
static_assert(category_count<int> == 1);
static_assert(category_count<const int&> == 1);
static_assert(category_count<foo&&> == 1);
static_assert(category_count<void> == 1);

} // namespace

int main() { return neutron_test::result(); }
