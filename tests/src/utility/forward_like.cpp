#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Length that does not fit into the small-string buffer.
constexpr std::size_t long_length = 64;

/// Owns a member to be forwarded like its owner.
struct owner {
    std::string member = std::string(long_length, 'm');
};

/// Result type of `forward_like<T>` applied to an expression of type `U`.
template <typename T, typename U>
using result_t = decltype(forward_like<T>(std::declval<U>()));

// Lvalue-reference `T` always yields an lvalue, keeping the argument's const.
static_assert(std::is_same_v<result_t<owner&, std::string&>, std::string&>);
static_assert(std::is_same_v<result_t<owner&, std::string>, std::string&>);
static_assert(
    std::is_same_v<result_t<owner&, const std::string&>, const std::string&>);
static_assert(std::is_same_v<result_t<int&, int>, int&>);

// Const lvalue-reference `T` yields a const lvalue.
static_assert(
    std::is_same_v<result_t<const owner&, std::string>, const std::string&>);
static_assert(std::is_same_v<
              result_t<const owner&, const std::string&>, const std::string&>);
static_assert(std::is_same_v<result_t<const int&, int>, const int&>);

// Rvalue-reference `T` with an rvalue argument yields an rvalue.
static_assert(std::is_same_v<result_t<owner&&, std::string>, std::string&&>);
static_assert(
    std::is_same_v<result_t<const owner&&, std::string>, const std::string&&>);

constexpr bool refers_to_the_argument_in_constexpr() {
    int value = 1;

    forward_like<int&>(value) = 2;
    const int& view           = forward_like<const int&>(std::move(value));

    return &view == &value && value == 2;
}
static_assert(refers_to_the_argument_in_constexpr());

/// Forwards a member of `self` with the value category of `Self`.
template <typename Self>
std::string take_member(Self&& self) {
    return forward_like<Self>(self.member);
}

void test_lvalue_forwarding_refers_to_the_argument() {
    const std::string expected(long_length, 'x');
    std::string text            = expected;
    const std::string& as_const = text;

    require(&forward_like<owner&>(text) == &text);
    require(&forward_like<owner&>(as_const) == &text);
    require(&forward_like<const owner&>(std::move(text)) == &text);
    // Forwarding as a const lvalue does not move from the argument.
    require(text == expected);
}

void test_member_of_lvalue_owner_is_copied() {
    owner source;

    std::string copied = take_member(source);

    require(copied == std::string(long_length, 'm'));
    require(source.member == copied);
}

void test_member_of_const_owner_is_copied() {
    const owner constant;

    std::string copied = take_member(constant);

    require(copied == std::string(long_length, 'm'));
}

void test_rvalue_forwarding_moves() {
    const std::string expected(long_length, 'y');
    std::string text = expected;

    std::string moved = forward_like<owner&&>(std::move(text));

    require(moved == expected);
}

} // namespace

int main() {
    test_lvalue_forwarding_refers_to_the_argument();
    test_member_of_lvalue_owner_is_copied();
    test_member_of_const_owner_is_copied();
    test_rvalue_forwarding_moves();
    return neutron_test::result();
}
