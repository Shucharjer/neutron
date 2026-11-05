#include <memory>
#include <tuple>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Empty class.
struct empty {};

/// Empty class derived from an empty base.
struct empty_derived : empty {};

/// Has a non-static data member.
struct with_member {
    int value;
};

/// A member of empty type still makes the class non-empty.
struct with_empty_member {
    empty value;
};

/// Static data members do not occupy storage.
struct with_static_member {
    [[maybe_unused]] static constexpr int value = 0;
};

/// Member functions do not occupy storage.
struct empty_with_functions {
    int get() const noexcept { return 0; }
};

/// Has a non-zero-width bit-field.
struct with_bitfield {
    int bits : 1;
};

/// Unions are never empty.
union small_union {
    int value;
};

// rejected: empty classes
static_assert(!nonempty<empty>);
static_assert(!nonempty<const empty>);
static_assert(!nonempty<empty_derived>);
static_assert(!nonempty<with_static_member>);
static_assert(!nonempty<empty_with_functions>);
static_assert(!nonempty<std::tuple<>>);
static_assert(!nonempty<std::allocator<int>>);
static_assert(!nonempty<std::default_delete<int>>);

// satisfied by: classes with storage
static_assert(nonempty<with_member>);
static_assert(nonempty<with_empty_member>);
static_assert(nonempty<with_bitfield>);
static_assert(nonempty<small_union>);
static_assert(nonempty<std::tuple<int>>);

// satisfied by: non-class types are never empty
static_assert(nonempty<int>);
static_assert(nonempty<int*>);
static_assert(nonempty<int[2]>);
static_assert(nonempty<empty&>);
static_assert(nonempty<empty*>);
static_assert(nonempty<void>);

} // namespace

int main() { return neutron_test::result(); }
