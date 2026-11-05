#include <memory>
#include <string>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Copyable without a move constructor, so moves fall back to copies.
struct copy_only {
    copy_only()                            = default;
    copy_only(const copy_only&)            = default;
    copy_only& operator=(const copy_only&) = default;
};

/// Neither movable nor copyable.
struct immovable {
    immovable()            = default;
    immovable(immovable&&) = delete;
};

/// Not destructible.
struct no_dtor {
    ~no_dtor() = delete;
};

/// Movable but not copyable.
using move_only = std::unique_ptr<int>;

// satisfied by
static_assert(movable_value<int>);
static_assert(movable_value<int*>);
static_assert(movable_value<std::string>);
static_assert(movable_value<move_only>);
static_assert(movable_value<copy_only>);

// references decay to their referenced type
static_assert(movable_value<int&>);
static_assert(movable_value<const int&>);
static_assert(movable_value<int&&>);
static_assert(movable_value<std::string&>);
static_assert(movable_value<move_only&&>);
static_assert(!movable_value<move_only&>);
static_assert(!movable_value<const move_only&>);

// cv-qualified values
static_assert(movable_value<const int>);
static_assert(movable_value<const std::string>);
static_assert(!movable_value<const move_only>);

// rejected
static_assert(!movable_value<immovable>);
static_assert(!movable_value<no_dtor>);
static_assert(!movable_value<void>);
static_assert(!movable_value<int[3]>);
static_assert(!movable_value<void()>);

} // namespace

int main() { return neutron_test::result(); }
