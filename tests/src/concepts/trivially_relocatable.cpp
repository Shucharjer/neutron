#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Scoped enumeration.
enum class color : unsigned char {
    red
};

/// Trivially copyable aggregate.
struct plain {
    int a;
    double b;
};

/// Aggregate of arrays, enums and pointers.
struct nested {
    plain inner[2];
    color tint;
    int* ptr;
};

/// Private data members do not affect relocatability.
struct private_members {
public:
    int get() const noexcept { return value_; }

private:
    int value_ = 0;
};

/// Explicitly defaulted special members stay trivial.
struct defaulted_ops {
    defaulted_ops()                                = default;
    defaulted_ops(const defaulted_ops&)            = default;
    defaulted_ops(defaulted_ops&&)                 = default;
    defaulted_ops& operator=(const defaulted_ops&) = default;
    defaulted_ops& operator=(defaulted_ops&&)      = default;
    ~defaulted_ops()                               = default;
    int value;
};

/// Non-virtual derivation from a trivial base.
struct derived_plain : plain {
    int extra;
};

/// Union of scalars.
union scalar_union {
    int i;
    float f;
};

/// User-provided move operations.
struct user_move {
    user_move() = default;
    user_move(user_move&&) noexcept {}
    user_move& operator=(user_move&&) noexcept { return *this; }
};

/// User-provided copy operations.
struct user_copy {
    user_copy() = default;
    user_copy(const user_copy&) {}
    user_copy& operator=(const user_copy&) { return *this; }
};

/// User-provided destructor.
struct user_dtor {
    ~user_dtor() {}
};

/// Member of a non-trivial standard library type.
struct holds_string {
    std::string value;
};

/// Member with user-provided moves.
struct holds_user_move {
    user_move value;
};

/// Base with user-provided moves.
struct derived_user_move : user_move {};

/// Virtual bases make copies non-trivial.
struct virtual_base : virtual plain {};

/// Only pointers to it are used.
struct incomplete;

// satisfied by: scalars
static_assert(trivially_relocatable<int>);
static_assert(trivially_relocatable<const int>);
static_assert(trivially_relocatable<volatile int>);
static_assert(trivially_relocatable<double>);
static_assert(trivially_relocatable<color>);
static_assert(trivially_relocatable<std::nullptr_t>);
static_assert(trivially_relocatable<int*>);
static_assert(trivially_relocatable<incomplete*>);
static_assert(trivially_relocatable<void (*)()>);
static_assert(trivially_relocatable<int plain::*>);

// satisfied by: trivially copyable classes
static_assert(trivially_relocatable<plain>);
static_assert(trivially_relocatable<const plain>);
static_assert(trivially_relocatable<nested>);
static_assert(trivially_relocatable<private_members>);
static_assert(trivially_relocatable<defaulted_ops>);
static_assert(trivially_relocatable<derived_plain>);
static_assert(trivially_relocatable<scalar_union>);

// satisfied by: arrays of relocatable elements
static_assert(trivially_relocatable<int[3]>);
static_assert(trivially_relocatable<plain[2][2]>);
static_assert(trivially_relocatable<const nested[4]>);

// rejected: non-object types
static_assert(!trivially_relocatable<void>);
static_assert(!trivially_relocatable<const void>);
static_assert(!trivially_relocatable<int&>);
static_assert(!trivially_relocatable<const plain&>);
static_assert(!trivially_relocatable<int&&>);
static_assert(!trivially_relocatable<void()>);

// rejected: user-provided special members, directly or through subobjects
static_assert(!trivially_relocatable<user_move>);
static_assert(!trivially_relocatable<user_copy>);
static_assert(!trivially_relocatable<user_dtor>);
static_assert(!trivially_relocatable<holds_user_move>);
static_assert(!trivially_relocatable<derived_user_move>);
static_assert(!trivially_relocatable<user_move[2]>);
static_assert(!trivially_relocatable<virtual_base>);

// rejected: standard library types with user-provided special members
static_assert(!trivially_relocatable<std::string>);
static_assert(!trivially_relocatable<std::unique_ptr<int>>);
static_assert(!trivially_relocatable<std::shared_ptr<int>>);
static_assert(!trivially_relocatable<std::vector<int>>);
static_assert(!trivially_relocatable<holds_string>);

} // namespace

int main() { return neutron_test::result(); }
