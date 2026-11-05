#include <memory>
#include <string>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Throwing move, nothrow copy.
struct nothrow_copy_only {
    nothrow_copy_only() = default;
    nothrow_copy_only(const nothrow_copy_only&) noexcept {}
    nothrow_copy_only(nothrow_copy_only&&) noexcept(false) {}
};

/// Deleted move, nothrow copy.
struct deleted_move {
    deleted_move() = default;
    deleted_move(const deleted_move&) noexcept {}
    deleted_move(deleted_move&&) = delete;
};

/// Both throwing.
struct throwing {
    throwing() = default;
    throwing(const throwing&) noexcept(false) {}
    throwing(throwing&&) noexcept(false) {}
};

/// Move-only and throwing.
struct throwing_move_only {
    throwing_move_only() = default;
    throwing_move_only(throwing_move_only&&) noexcept(false) {}
};

/// Nothrow move, throwing copy.
struct nothrow_move {
    nothrow_move() = default;
    nothrow_move(const nothrow_move&) noexcept(false) {}
    nothrow_move(nothrow_move&&) noexcept {}
};

/// Neither movable nor copyable.
struct immovable {
    immovable()            = default;
    immovable(immovable&&) = delete;
};

// satisfied by: nothrow move or, failing that, nothrow copy
static_assert(nothrow_conditional_move_constrctible<int>);
static_assert(nothrow_conditional_move_constrctible<int*>);
static_assert(nothrow_conditional_move_constrctible<std::string>);
static_assert(nothrow_conditional_move_constrctible<std::vector<int>>);
static_assert(nothrow_conditional_move_constrctible<std::unique_ptr<int>>);
static_assert(nothrow_conditional_move_constrctible<nothrow_move>);
static_assert(nothrow_conditional_move_constrctible<nothrow_copy_only>);
static_assert(nothrow_conditional_move_constrctible<deleted_move>);
static_assert(nothrow_conditional_move_constrctible<const int>);

// satisfied by: references, which are bound rather than moved
static_assert(nothrow_conditional_move_constrctible<int&>);
static_assert(nothrow_conditional_move_constrctible<throwing&>);

// rejected
static_assert(!nothrow_conditional_move_constrctible<throwing>);
static_assert(!nothrow_conditional_move_constrctible<throwing_move_only>);
static_assert(!nothrow_conditional_move_constrctible<immovable>);
static_assert(
    !nothrow_conditional_move_constrctible<const std::unique_ptr<int>>);
static_assert(!nothrow_conditional_move_constrctible<void>);
static_assert(!nothrow_conditional_move_constrctible<int[3]>);
static_assert(!nothrow_conditional_move_constrctible<void()>);

// a const nothrow-move type falls back to a throwing copy
static_assert(!nothrow_conditional_move_constrctible<const nothrow_move>);
static_assert(nothrow_conditional_move_constrctible<const nothrow_copy_only>);

} // namespace

int main() { return neutron_test::result(); }
