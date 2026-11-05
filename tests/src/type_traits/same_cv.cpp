#include <string>
#include <type_traits>
#include <utility>
#include <neutron/type_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Both the alias and the `::type` member name `Expected`.
template <typename To, typename From, typename Expected>
constexpr bool cv_is =
    std::is_same_v<same_cv_t<To, From>, Expected> &&
    std::is_same_v<typename same_cv<To, From>::type, Expected>;

/// Checks every qualifier combination of `From` for an unqualified `To`.
template <typename To, typename From>
constexpr bool copies_cv() {
    // Top-level const and volatile are copied.
    static_assert(cv_is<To, From, To>);
    static_assert(cv_is<To, const From, const To>);
    static_assert(cv_is<To, volatile From, volatile To>);
    static_assert(cv_is<To, const volatile From, const volatile To>);
    // References carry no top-level cv.
    static_assert(cv_is<To, From&, To>);
    static_assert(cv_is<To, From&&, To>);
    static_assert(cv_is<To, const From&, To>);
    static_assert(cv_is<To, const From&&, To>);
    static_assert(cv_is<To, volatile From&, To>);
    static_assert(cv_is<To, const volatile From&, To>);
    // Only the cv of the pointer itself counts, not of the pointee.
    static_assert(cv_is<To, const From*, To>);
    static_assert(cv_is<To, From* const volatile, const volatile To>);
    return true;
}

/// Empty class type.
struct object {};

/// Owner whose cv the accessor below mirrors.
struct holder {
    int value = 3;
};

/// Returns a pointer to the member, qualified like the owner.
template <typename Owner>
same_cv_t<int, Owner>* address_of(Owner& owner) {
    return &owner.value;
}

// Fundamental, class and pointer types on both sides
static_assert(copies_cv<int, char>());
static_assert(copies_cv<int, std::string>());
static_assert(copies_cv<std::string, int>());
static_assert(copies_cv<object, object>());
static_assert(copies_cv<int*, double>());
static_assert(copies_cv<const char*, int>());

// Explicit spot checks
static_assert(std::is_same_v<same_cv_t<int, char>, int>);
static_assert(std::is_same_v<same_cv_t<int, const char>, const int>);
static_assert(std::is_same_v<same_cv_t<int, volatile char>, volatile int>);
static_assert(
    std::is_same_v<same_cv_t<int, const volatile char>, const volatile int>);

// Existing qualifiers on the destination are merged.
static_assert(
    std::is_same_v<same_cv_t<const int, volatile char>, const volatile int>);
static_assert(
    std::is_same_v<same_cv_t<volatile int, const char>, const volatile int>);
static_assert(std::is_same_v<
              same_cv_t<const volatile int, const volatile char>,
              const volatile int>);
static_assert(std::is_same_v<same_cv_t<const int, char>, const int>);

// cv applied to a pointer qualifies the pointer itself.
static_assert(std::is_same_v<same_cv_t<int*, const char>, int* const>);
static_assert(std::is_same_v<same_cv_t<int*, volatile char>, int* volatile>);

// cv applied to a reference is ignored.
static_assert(std::is_same_v<same_cv_t<int&, const volatile char>, int&>);

// Arrays of cv elements are cv-qualified types.
static_assert(std::is_same_v<same_cv_t<int, const char[2]>, const int>);
static_assert(
    std::is_same_v<same_cv_t<int, const volatile char[2]>, const volatile int>);

// The accessor mirrors the cv of its owner.
static_assert(
    std::is_same_v<decltype(address_of(std::declval<holder&>())), int*>);
static_assert(std::is_same_v<
              decltype(address_of(std::declval<const holder&>())), const int*>);
static_assert(
    std::is_same_v<
        decltype(address_of(std::declval<volatile holder&>())), volatile int*>);

void test_pointer_writes_through_mutable_owner() {
    holder plain;

    *address_of(plain) = 4;

    require(plain.value == 4);
}

void test_pointer_reads_const_owner() {
    const holder constant{};

    require(*address_of(constant) == 3);
}

void test_pointer_reads_volatile_owner() {
    volatile holder vol{};

    require(*address_of(vol) == 3);
}

} // namespace

int main() {
    test_pointer_writes_through_mutable_owner();
    test_pointer_reads_const_owner();
    test_pointer_reads_volatile_owner();
    return neutron_test::result();
}
