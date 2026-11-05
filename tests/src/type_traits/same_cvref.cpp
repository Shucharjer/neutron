#include <string>
#include <type_traits>
#include <utility>
#include <neutron/type_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Both the alias and the `::type` member name `Expected`.
template <typename To, typename From, typename Expected>
constexpr bool cvref_is =
    std::is_same_v<same_cvref_t<To, From>, Expected> &&
    std::is_same_v<typename same_cvref<To, From>::type, Expected>;

/// Checks const and reference combinations of `From` for an unqualified `To`.
template <typename To, typename From>
constexpr bool copies_cvref() {
    static_assert(cvref_is<To, From, To>);
    static_assert(cvref_is<To, const From, const To>);
    static_assert(cvref_is<To, From&, To&>);
    static_assert(cvref_is<To, From&&, To&&>);
    static_assert(cvref_is<To, const From&, const To&>);
    static_assert(cvref_is<To, const From&&, const To&&>);
    return true;
}

/// Empty class type.
struct object {};

/// Owner whose member is forwarded below.
struct holder {
    std::string value = "member";
};

/// Forwards a member with the value category of its owner.
template <typename Owner>
same_cvref_t<std::string, Owner&&> member_of(Owner&& owner) {
    return static_cast<same_cvref_t<std::string, Owner&&>>(owner.value);
}

// Const and reference qualifiers of `From` are copied.
static_assert(copies_cvref<int, char>());
static_assert(copies_cvref<int, std::string>());
static_assert(copies_cvref<std::string, int>());
static_assert(copies_cvref<object, object>());
static_assert(copies_cvref<int*, double>());

// Explicit spot checks
static_assert(std::is_same_v<same_cvref_t<int, char>, int>);
static_assert(std::is_same_v<same_cvref_t<int, const char>, const int>);
static_assert(std::is_same_v<same_cvref_t<int, char&>, int&>);
static_assert(std::is_same_v<same_cvref_t<int, char&&>, int&&>);
static_assert(std::is_same_v<same_cvref_t<int, const char&>, const int&>);
static_assert(std::is_same_v<same_cvref_t<int, const char&&>, const int&&>);

// Pointers: only top-level qualifiers of `From` matter.
static_assert(std::is_same_v<same_cvref_t<int, const char*>, int>);
static_assert(std::is_same_v<same_cvref_t<int, char* const>, const int>);
static_assert(std::is_same_v<same_cvref_t<int, const char*&>, int&>);

// Const on the destination is kept.
static_assert(std::is_same_v<same_cvref_t<const int, char&>, const int&>);
static_assert(std::is_same_v<same_cvref_t<const int, char&&>, const int&&>);

// References on the destination follow reference collapsing.
static_assert(std::is_same_v<same_cvref_t<int&, char&&>, int&>);
static_assert(std::is_same_v<same_cvref_t<int&&, char&>, int&>);
static_assert(std::is_same_v<same_cvref_t<int&&, char&&>, int&&>);
static_assert(std::is_same_v<same_cvref_t<int&, char>, int&>);

// Typical use: forward a member with the value category of its owner.
static_assert(
    std::is_same_v<decltype(member_of(std::declval<holder&>())), std::string&>);
static_assert(std::is_same_v<
              decltype(member_of(std::declval<const holder&>())),
              const std::string&>);
static_assert(std::is_same_v<
              decltype(member_of(std::declval<holder&&>())), std::string&&>);
static_assert(std::is_same_v<
              decltype(member_of(std::declval<const holder&&>())),
              const std::string&&>);

void test_lvalue_owners_yield_the_member_itself() {
    holder lvalue;
    const holder constant{};

    require(&member_of(lvalue) == &lvalue.value);
    require(&member_of(constant) == &constant.value);
}

void test_rvalue_owner_moves_the_member_out() {
    holder owner;

    std::string moved = member_of(std::move(owner));

    require(moved == "member");
}

void test_const_rvalue_owner_is_not_moved_from() {
    const holder constant{};

    // A const rvalue binds to the copy constructor.
    std::string copied = member_of(std::move(constant));

    require(copied == "member");
    require(constant.value == "member");
}

} // namespace

int main() {
    test_lvalue_owners_yield_the_member_itself();
    test_rvalue_owner_moves_the_member_out();
    test_const_rvalue_owner_is_not_moved_from();
    return neutron_test::result();
}
