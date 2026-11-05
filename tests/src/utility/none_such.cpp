#include <type_traits>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Satisfied when `T` is a complete type.
template <typename T>
concept complete = requires { sizeof(T); };

/// Typical use: a placeholder default that never matches a real type.
template <typename T = none_such>
constexpr bool specified = !std::is_same_v<T, none_such>;

// `none_such` is a declared but never defined class.
static_assert(std::is_class_v<none_such>);
static_assert(!complete<none_such>);
static_assert(!std::is_same_v<none_such, void>);

// It can still be named through pointers and references.
static_assert(std::is_pointer_v<none_such*>);
static_assert(std::is_reference_v<none_such&>);

// The default is distinguishable from any real argument.
static_assert(!specified<>);
static_assert(specified<int>);

void test_pointers_to_it_can_be_held() {
    none_such* pointer = nullptr;

    const none_such* const_pointer = pointer;

    require(pointer == nullptr);
    require(const_pointer == nullptr);
}

} // namespace

int main() {
    test_pointers_to_it_can_be_held();
    return neutron_test::result();
}
