#include <map>
#include <tuple>
#include <utility>
#include <neutron/concepts.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Empty class, also used as a compressible pair element.
struct empty {};

/// Public members with typedefs.
struct public_kv {
    using first_type  = int;
    using second_type = float;
    int first;
    float second;
};

/// Accessors with typedefs.
class private_kv {
public:
    using first_type  = int;
    using second_type = float;
    int first() const noexcept { return key_; }
    float second() const noexcept { return value_; }

private:
    int key_     = 0;
    float value_ = 0;
};

/// Accessors only for non-const objects.
struct mutable_accessors {
    using first_type  = int;
    using second_type = int;
    int& first() noexcept;
    int& second() noexcept;
};

/// Public members without typedefs.
struct untyped_members {
    int first;
    int second;
};

/// Static members are not member objects.
struct static_members {
    using first_type  = int;
    using second_type = int;
    [[maybe_unused]] static int first;
    [[maybe_unused]] static int second;
};

/// Only one member.
struct half_pair {
    using first_type  = int;
    using second_type = int;
    int first;
};

// std::pair is a public pair
static_assert(public_pair<std::pair<int, float>>);
static_assert(public_pair<const std::pair<int, float>>);
static_assert(public_pair<std::pair<const int, float>>);
static_assert(public_pair<std::pair<int*, empty>>);
static_assert(!private_pair<std::pair<int, float>>);
static_assert(pair<std::pair<int, float>>);
static_assert(pair<std::map<int, int>::value_type>);

// user defined pair with public members
static_assert(public_pair<public_kv>);
static_assert(!private_pair<public_kv>);
static_assert(pair<public_kv>);

// user defined pair with accessors
static_assert(private_pair<private_kv>);
static_assert(!public_pair<private_kv>);
static_assert(pair<private_kv>);
static_assert(pair<const private_kv>);

// compressed_pair exposes accessors
static_assert(private_pair<compressed_pair<int, float>>);
static_assert(private_pair<compressed_pair<empty, int>>);
static_assert(!public_pair<compressed_pair<int, float>>);
static_assert(pair<compressed_pair<empty, empty>>);

// rejected
static_assert(!pair<mutable_accessors>);
static_assert(!pair<untyped_members>);
static_assert(!pair<static_members>);
static_assert(!pair<half_pair>);
static_assert(!pair<std::tuple<int, float>>);
static_assert(!pair<int>);
static_assert(!pair<void>);
static_assert(!pair<empty>);

// rejected: references, pointers and arrays
static_assert(!pair<std::pair<int, int>&>);
static_assert(!pair<const std::pair<int, int>&>);
static_assert(!pair<std::pair<int, int>*>);
static_assert(!pair<std::pair<int, int>[2]>);
static_assert(!pair<private_kv&>);

} // namespace

int main() { return neutron_test::result(); }
