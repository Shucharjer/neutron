#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>

using namespace neutron;

namespace {

struct empty {};
struct final_empty final {};

template <typename Ty>
using element = _compressed_element<Ty, 0>;

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
static_assert(
    std::is_nothrow_constructible_v<element<int>, std::in_place_t, int>);
static_assert(!std::is_nothrow_constructible_v<
              element<std::string>, std::in_place_t, const char*>);
static_assert(!std::is_default_constructible_v<element<int&>>);

// `value()` propagates the value category, references collapse.
static_assert(
    std::is_same_v<decltype(std::declval<element<int>&>().value()), int&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const element<int>&>().value()), const int&>);
static_assert(
    std::is_same_v<decltype(std::declval<element<int>&&>().value()), int&&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const element<int>&&>().value()), const int&&>);
static_assert(
    std::is_same_v<decltype(std::declval<element<int&>&&>().value()), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<element<int*>&&>().value()), int*&&>);

constexpr bool test_constexpr() {
    element<int> value{ std::in_place, 3 };
    element<int> copy = value;
    copy.value()      = 4;
    element<empty> nothing{ std::in_place };
    static_cast<void>(nothing.value());
    return value.value() == 3 && copy.value() == 4;
}
static_assert(test_constexpr());

bool test_runtime() {
    element<std::string> text{ std::in_place, 3, 'x' };
    element<std::string> moved{ std::move(text) };
    std::string taken = std::move(moved).value();

    int target = 1;
    element<int&> ref{ std::in_place, target };
    ref.value() = 2;
    return taken == "xxx" && moved.value().empty() && target == 2;
}

} // namespace

int main() { return test_runtime() ? 0 : 1; }
