#include <string>
#include <type_traits>
#include <utility>
#include <neutron/type_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Both the alias and the `::type` member name `Expected`.
template <typename To, typename From, typename Expected>
constexpr bool constness_is =
    std::is_same_v<same_constness_t<To, From>, Expected> &&
    std::is_same_v<typename same_constness<To, From>::type, Expected>;

/// Checks every qualifier combination of `From` for an unqualified `To`.
template <typename To, typename From>
constexpr bool copies_constness() {
    // Only a top-level const on `From` is copied; volatile is not.
    static_assert(constness_is<To, From, To>);
    static_assert(constness_is<To, const From, const To>);
    static_assert(constness_is<To, volatile From, To>);
    static_assert(constness_is<To, const volatile From, const To>);
    // References carry no top-level const.
    static_assert(constness_is<To, From&, To>);
    static_assert(constness_is<To, From&&, To>);
    static_assert(constness_is<To, const From&, To>);
    static_assert(constness_is<To, const From&&, To>);
    // Only the constness of the pointer itself counts, not of the pointee.
    static_assert(constness_is<To, const From*, To>);
    static_assert(constness_is<To, From* const, const To>);
    return true;
}

/// Empty class type.
struct object {};

/// Owner whose constness the accessor below mirrors.
struct holder {
    int value = 1;
};

/// Useful for building accessors that mirror the constness of an owner.
template <typename Owner>
same_constness_t<int, Owner>& value_of(Owner& owner) {
    return owner.value;
}

// Fundamental, class and pointer types on both sides
static_assert(copies_constness<int, char>());
static_assert(copies_constness<int, std::string>());
static_assert(copies_constness<std::string, int>());
static_assert(copies_constness<object, object>());
static_assert(copies_constness<int*, double>());
static_assert(copies_constness<const char*, int>());

// Explicit spot checks
static_assert(std::is_same_v<same_constness_t<int, const char>, const int>);
static_assert(std::is_same_v<same_constness_t<int, char>, int>);
static_assert(
    std::is_same_v<same_constness_t<int, const volatile char>, const int>);
static_assert(std::is_same_v<same_constness_t<int, volatile char>, int>);

// Const on the destination is kept and not duplicated.
static_assert(std::is_same_v<same_constness_t<const int, char>, const int>);
static_assert(
    std::is_same_v<same_constness_t<const int, const char>, const int>);
static_assert(std::is_same_v<
              same_constness_t<volatile int, const char>, const volatile int>);

// Const applied to a pointer qualifies the pointer itself.
static_assert(std::is_same_v<same_constness_t<int*, const char>, int* const>);

// Const applied to a reference is ignored.
static_assert(std::is_same_v<same_constness_t<int&, const char>, int&>);

// Arrays of const elements are const-qualified types.
static_assert(std::is_same_v<same_constness_t<int, const char[3]>, const int>);
static_assert(std::is_same_v<same_constness_t<int, char[3]>, int>);

// The accessor mirrors the constness of its owner.
static_assert(
    std::is_same_v<decltype(value_of(std::declval<holder&>())), int&>);
static_assert(std::is_same_v<
              decltype(value_of(std::declval<const holder&>())), const int&>);

void test_accessor_writes_through_mutable_owner() {
    holder mutable_holder;

    value_of(mutable_holder) = 5;

    require(mutable_holder.value == 5);
}

void test_accessor_reads_const_owner() {
    const holder const_holder{};

    require(value_of(const_holder) == 1);
}

} // namespace

int main() {
    test_accessor_writes_through_mutable_owner();
    test_accessor_reads_const_owner();
    return neutron_test::result();
}
