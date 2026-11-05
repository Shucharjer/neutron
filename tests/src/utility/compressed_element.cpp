#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Empty type eligible for the empty-base optimisation.
struct empty {};

/// Empty type that cannot be used as a base class.
struct final_empty final {};

template <typename Ty>
using element = _compressed_element<Ty, 0>;

/// Result type of `value()` called on an `element<Ty>` of type `Self`.
template <typename Self>
using value_result = decltype(std::declval<Self>().value());

// Member types
static_assert(std::is_same_v<element<int>::value_type, int>);
static_assert(std::is_same_v<element<int>::reference, int&>);
static_assert(std::is_same_v<element<int>::const_reference, const int&>);

// Empty types are stored without taking space; others keep their layout.
static_assert(std::is_empty_v<element<empty>>);
static_assert(std::is_empty_v<element<final_empty>>);
static_assert(sizeof(element<int>) == sizeof(int));
static_assert(sizeof(element<std::string>) == sizeof(std::string));
static_assert(alignof(element<double>) == alignof(double));

// Special members are defaulted, keeping the triviality of the element.
static_assert(std::is_trivially_copyable_v<element<int>>);
static_assert(std::is_trivially_copyable_v<element<int*>>);
static_assert(std::is_trivially_copyable_v<element<empty>>);
static_assert(std::is_trivially_default_constructible_v<element<int>>);
static_assert(!std::is_trivially_copyable_v<element<std::string>>);
static_assert(!std::is_copy_constructible_v<element<std::unique_ptr<int>>>);
static_assert(
    std::is_nothrow_move_constructible_v<element<std::unique_ptr<int>>>);

// Values are only constructed through `std::in_place`, so the constructor
// never competes with copy and move.
static_assert(std::is_constructible_v<
              element<std::string>, std::in_place_t, const char*>);
static_assert(!std::is_constructible_v<element<std::string>, const char*>);
static_assert(
    !std::is_constructible_v<element<int>, std::in_place_t, std::string>);
static_assert(!std::is_convertible_v<std::in_place_t, element<int>>);
static_assert(!std::is_default_constructible_v<element<int&>>);

// The in-place constructor is noexcept exactly when the element's is.
static_assert(
    std::is_nothrow_constructible_v<element<int>, std::in_place_t, int>);
static_assert(!std::is_nothrow_constructible_v<
              element<std::string>, std::in_place_t, const char*>);

// `value()` propagates the value category; references collapse.
static_assert(std::is_same_v<value_result<element<int>&>, int&>);
static_assert(std::is_same_v<value_result<const element<int>&>, const int&>);
static_assert(std::is_same_v<value_result<element<int>&&>, int&&>);
static_assert(std::is_same_v<value_result<const element<int>&&>, const int&&>);
static_assert(std::is_same_v<value_result<element<int&>&&>, int&>);
static_assert(std::is_same_v<value_result<element<int*>&&>, int*&&>);

constexpr bool copy_is_independent_in_constexpr() {
    element<int> value{ std::in_place, 3 };
    element<int> copy = value;

    copy.value() = 4;

    return value.value() == 3 && copy.value() == 4;
}
static_assert(copy_is_independent_in_constexpr());

constexpr bool empty_value_is_accessible_in_constexpr() {
    element<empty> nothing{ std::in_place };
    static_cast<void>(nothing.value());
    return true;
}
static_assert(empty_value_is_accessible_in_constexpr());

void test_rvalue_value_moves_out_the_string() {
    element<std::string> text{ std::in_place, 3, 'x' };
    element<std::string> moved{ std::move(text) };

    std::string taken = std::move(moved).value();

    require(taken == "xxx");
    require(moved.value().empty());
}

void test_reference_element_writes_through() {
    int target = 1;
    element<int&> ref{ std::in_place, target };

    ref.value() = 2;

    require(target == 2);
}

} // namespace

int main() {
    test_rvalue_value_moves_out_the_string();
    test_reference_element_writes_through();
    return neutron_test::result();
}
