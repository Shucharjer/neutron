#include <cstddef>
#include <string>
#include <neutron/type_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Provides member, const member, static member and data member pointers.
struct object {
    int value = 0;
    int member(int) { return value; }
    int const_member(int) const { return value; }
    static int static_member(int arg) { return arg; }
};

int free_fn(int arg) { return arg; }
void nullary_fn() {}

/// Captureless closure, convertible to `int (*)(int)`.
inline constexpr auto captureless = [](int arg) {
    return arg;
};

/// Pointer to the function type `T`.
template <typename T>
using fp = T*;

// Plain function pointers of various signatures
static_assert(is_function_pointer_v<void (*)()>);
static_assert(is_function_pointer_v<int (*)(int)>);
static_assert(is_function_pointer_v<int& (*)(const int&, int&&)>);
static_assert(is_function_pointer_v<std::string (*)(std::string, char*)>);
static_assert(is_function_pointer_v<decltype(&free_fn)>);
static_assert(is_function_pointer_v<decltype(&nullary_fn)>);
static_assert(is_function_pointer_v<decltype(&object::static_member)>);
static_assert(is_function_pointer_v<fp<void(int, double, long)>>);
static_assert(is_function_pointer<int (*)(int)>::value);

// Function types and references to functions are not pointers.
static_assert(!is_function_pointer_v<void()>);
static_assert(!is_function_pointer_v<int(int)>);
static_assert(!is_function_pointer_v<int (&)(int)>);
static_assert(!is_function_pointer_v<int (&&)(int)>);

// A reference to a function pointer is not a function pointer.
static_assert(!is_function_pointer_v<int (*&)(int)>);

// Member pointers are not function pointers.
static_assert(!is_function_pointer_v<decltype(&object::member)>);
static_assert(!is_function_pointer_v<decltype(&object::const_member)>);
static_assert(!is_function_pointer_v<decltype(&object::value)>);

// Object pointers and other non-function types
static_assert(!is_function_pointer_v<void*>);
static_assert(!is_function_pointer_v<const void*>);
static_assert(!is_function_pointer_v<int*>);
static_assert(!is_function_pointer_v<int**>);
static_assert(!is_function_pointer_v<int (**)(int)>);
static_assert(!is_function_pointer_v<std::nullptr_t>);
static_assert(!is_function_pointer_v<int>);
static_assert(!is_function_pointer_v<object>);

// Closures are not function pointers, even when they are convertible to one.
static_assert(!is_function_pointer_v<decltype(captureless)>);

// The trait is usable in constant expressions.
constexpr bool value = is_function_pointer_v<void (*)()>;
static_assert(value);

/// Dispatches on the trait; only function pointers are called directly.
template <typename Fn>
int call_if_pointer(Fn fn, int arg) {
    if constexpr (is_function_pointer_v<Fn>) {
        return fn(arg);
    } else {
        return -1;
    }
}

void test_function_pointers_are_called() {
    // Only named in unevaluated contexts otherwise.
    nullary_fn();

    require(call_if_pointer(&free_fn, 7) == 7);
    require(call_if_pointer(&object::static_member, 9) == 9);
}

void test_closures_are_not_called() {
    require(call_if_pointer(captureless, 3) == -1);
}

} // namespace

int main() {
    test_function_pointers_are_called();
    test_closures_are_not_called();
    return neutron_test::result();
}
