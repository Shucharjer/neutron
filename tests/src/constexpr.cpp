#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <neutron/constexpr.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Functions evaluable in constant expressions.
constexpr int constant_unary(int value) { return value + 1; }
constexpr int constant_binary(int lhs, double rhs) {
    return lhs + static_cast<int>(rhs);
}
constexpr void constant_void() {}
constexpr int constant_nullary() { return 42; }

/// Functions that are never constant.
int runtime_unary(int value) { return value + std::rand(); }
int runtime_nullary() { return std::rand(); }

/// A constexpr function whose evaluation with default arguments fails.
constexpr int conditionally_constant(int value) {
    if (value == 0) {
        return runtime_nullary();
    }
    return value;
}

/// Evaluates `Fn` at compile time when possible, -1 otherwise.
template <auto Fn>
constexpr int evaluate_or_default() {
    if constexpr (is_constexpr<Fn>()) {
        constexpr int value = Fn();
        return value;
    } else {
        return -1;
    }
}

// Free functions
static_assert(is_constexpr<&constant_unary>());
static_assert(is_constexpr<&constant_binary>());
static_assert(is_constexpr<&constant_void>());
static_assert(is_constexpr<&constant_nullary>());
static_assert(!is_constexpr<&runtime_unary>());
static_assert(!is_constexpr<&runtime_nullary>());
static_assert(!is_constexpr<&conditionally_constant>());

// Lambdas (non-mutable, non-noexcept call operators).
static_assert(is_constexpr<[] {
    return 1;
}>());
static_assert(is_constexpr<[](int value) {
    return value * 2;
}>());
static_assert(is_constexpr<[](int, char, double) {
}>());
static_assert(!is_constexpr<[] {
    return std::rand();
}>());
static_assert(!is_constexpr<[](int value) {
    return runtime_unary(value);
}>());

// Usable as a compile-time switch.
static_assert(evaluate_or_default<&constant_nullary>() == 42);
static_assert(evaluate_or_default<[] {
                  return 7;
              }>() == 7);

// Argument metadata of the underlying traits
static_assert(internal::_expression_traits<&constant_unary>::args_count == 1);
static_assert(internal::_expression_traits<&constant_binary>::args_count == 2);
static_assert(std::is_same_v<
              internal::_expression_traits<&constant_binary>::args_type,
              type_list<int, double>>);
static_assert(internal::_expression_traits<&constant_void>::args_count == 0);
// MSVC fails to parse `traits<lambda>::args_type` as a template type argument,
// so name the traits through an alias first.
using lambda_traits = internal::_expression_traits<[](int, char) {
    return 0;
}>;
static_assert(lambda_traits::args_count == 2);
static_assert(std::is_same_v<lambda_traits::args_type, type_list<int, char>>);

void test_non_constant_callable_takes_the_runtime_branch() {
    const int value = evaluate_or_default<&runtime_nullary>();

    require(value == -1);
}

void test_non_constant_functions_remain_callable() {
    // std::rand() never returns a negative value.
    require(runtime_unary(0) >= 0);
    require(conditionally_constant(3) == 3);
}

} // namespace

int main() {
    test_non_constant_callable_takes_the_runtime_branch();
    test_non_constant_functions_remain_callable();
    return neutron_test::result();
}
