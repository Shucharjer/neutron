#include <string>
#include <neutron/type_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

int free_fn(int arg) { return arg + 1; }

/// A functor with a call operator and a conversion to a matching pointer.
struct convertible_functor {
    using pointer = int (*)(int);
    static int invoke(int arg) { return arg * 2; }
    int operator()(int arg) const { return invoke(arg); }
    operator pointer() const { return &invoke; }
};

/// A functor with a call operator but without any conversion.
struct plain_functor {
    int operator()(int arg) const { return arg; }
};

/// A functor whose conversion does not match the call operator.
struct mismatched_functor {
    using pointer = long (*)(long);
    static long invoke(long arg) { return arg; }
    int operator()(int arg) const { return arg; }
    operator pointer() const { return &invoke; }
};

/// Overloaded call operators make `&T::operator()` ambiguous.
struct overloaded_functor {
    int operator()(int arg) const { return arg; }
    int operator()(double arg) const { return static_cast<int>(arg); }
};

/// Not callable at all.
struct no_call_operator {};

/// Captureless lambdas with every combination of mutable and noexcept.
inline auto captureless = [](int arg) {
    return arg;
};
inline auto captureless_void = [] {
};
inline auto captureless_multi = [](int, const std::string&,
                                   double&&) -> std::string& {
    static std::string storage;
    return storage;
};
inline auto captureless_noexcept = [](int arg) noexcept {
    return arg;
};
inline auto captureless_mutable = [](int arg) mutable {
    return arg;
};
inline auto captureless_mutable_noexcept = [](int arg) mutable noexcept {
    return arg;
};
/// Generic lambda.
inline auto generic = [](auto arg) {
    return arg;
};

// Plain function pointers
static_assert(is_convertible_to_function_pointer_v<void (*)()>);
static_assert(is_convertible_to_function_pointer_v<int (*)(int)>);
static_assert(is_convertible_to_function_pointer_v<decltype(&free_fn)>);
static_assert(is_convertible_to_function_pointer<int (*)(int)>::value);

// Captureless lambdas convert, whatever their qualifiers.
static_assert(is_convertible_to_function_pointer_v<decltype(captureless)>);
static_assert(is_convertible_to_function_pointer_v<decltype(captureless_void)>);
static_assert(
    is_convertible_to_function_pointer_v<decltype(captureless_multi)>);
static_assert(
    is_convertible_to_function_pointer_v<decltype(captureless_noexcept)>);
static_assert(
    is_convertible_to_function_pointer_v<decltype(captureless_mutable)>);
static_assert(is_convertible_to_function_pointer_v<
              decltype(captureless_mutable_noexcept)>);

// Functors need a conversion matching their only call operator.
static_assert(is_convertible_to_function_pointer_v<convertible_functor>);
static_assert(!is_convertible_to_function_pointer_v<plain_functor>);
static_assert(!is_convertible_to_function_pointer_v<mismatched_functor>);
static_assert(!is_convertible_to_function_pointer_v<overloaded_functor>);
static_assert(!is_convertible_to_function_pointer_v<no_call_operator>);

// Generic lambdas have no single call operator.
static_assert(!is_convertible_to_function_pointer_v<decltype(generic)>);

// Other types, including function types, references and member pointers
static_assert(!is_convertible_to_function_pointer_v<int>);
static_assert(!is_convertible_to_function_pointer_v<void*>);
static_assert(!is_convertible_to_function_pointer_v<int*>);
static_assert(!is_convertible_to_function_pointer_v<int(int)>);
static_assert(!is_convertible_to_function_pointer_v<int (&)(int)>);
static_assert(!is_convertible_to_function_pointer_v<
              decltype(&plain_functor::operator())>);

/// Converts only when the trait says it is possible; -1 otherwise.
template <typename Fn>
int call_through_pointer(Fn fn, int arg) {
    if constexpr (is_convertible_to_function_pointer_v<Fn>) {
        int (*ptr)(int) = fn;
        return ptr(arg);
    } else {
        return -1;
    }
}

void test_capturing_lambdas_do_not_convert() {
    int state     = 1;
    auto by_value = [state](int arg) {
        return state + arg;
    };
    auto by_reference = [&state](int arg) {
        return state + arg;
    };
    auto init_capture = [copy = std::string("x")] {
        return copy.size();
    };
    auto mutable_capture = [state](int arg) mutable {
        state += arg;
        return state;
    };

    static_assert(!is_convertible_to_function_pointer_v<decltype(by_value)>);
    static_assert(
        !is_convertible_to_function_pointer_v<decltype(by_reference)>);
    static_assert(
        !is_convertible_to_function_pointer_v<decltype(init_capture)>);
    static_assert(
        !is_convertible_to_function_pointer_v<decltype(mutable_capture)>);

    // They remain ordinary callables.
    require(by_value(1) == 2);
    require(by_reference(2) == 3);
    require(init_capture() == 1);
    require(mutable_capture(4) == 5);
}

void test_convertible_callables_are_called_through_pointer() {
    auto copy = captureless_mutable;

    require(call_through_pointer(&free_fn, 1) == 2);
    require(call_through_pointer(captureless, 5) == 5);
    require(call_through_pointer(captureless_noexcept, 6) == 6);
    require(call_through_pointer(copy, 7) == 7);
    require(call_through_pointer(captureless_mutable_noexcept, 8) == 8);
    require(call_through_pointer(convertible_functor{}, 4) == 8);
}

void test_unconvertible_functor_is_not_converted() {
    require(call_through_pointer(plain_functor{}, 4) == -1);
}

void test_void_lambda_converts_to_nullary_pointer() {
    captureless_void();

    void (*nullary)() = captureless_void;

    require(nullary != nullptr);
    nullary();
}

void test_unconvertible_callables_remain_callable() {
    require(generic(3) == 3);
    require(overloaded_functor{}(2.0) == 2);
    require(mismatched_functor{}(1) == 1);
    require(captureless_multi(0, "", 0.0) == "");
}

} // namespace

int main() {
    test_capturing_lambdas_do_not_convert();
    test_convertible_callables_are_called_through_pointer();
    test_unconvertible_functor_is_not_converted();
    test_void_lambda_converts_to_nullary_pointer();
    test_unconvertible_callables_remain_callable();
    return neutron_test::result();
}
