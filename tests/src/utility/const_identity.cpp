#include <concepts>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Copy operations may throw, move operations may not.
struct throwing_copy {
    throwing_copy() = default;
    throwing_copy(const throwing_copy&) noexcept(false) {}
    throwing_copy(throwing_copy&&) noexcept = default;
    throwing_copy& operator=(const throwing_copy&) noexcept(false) {
        return *this;
    }
    throwing_copy& operator=(throwing_copy&&) noexcept = default;
};

/// Satisfied when `wrapper.assign(value)` is well-formed.
template <typename Wrapper, typename T>
concept can_assign = requires(Wrapper& wrapper, T&& value) {
    wrapper.assign(std::forward<T>(value));
};

using int_identity    = const_identity<int>;
using string_identity = const_identity<std::string>;

/// Length that does not fit into the small-string buffer.
constexpr std::size_t long_length = 64;

/// Reads the wrapped string by value.
std::string text_of(const string_identity& identity) {
    return static_cast<std::string>(identity);
}

// Construction and assignment
static_assert(std::is_default_constructible_v<int_identity>);
static_assert(std::is_constructible_v<int_identity, const int&>);
static_assert(std::is_constructible_v<int_identity, int&&>);
static_assert(std::is_convertible_v<int, int_identity>);
static_assert(std::is_convertible_v<std::string, string_identity>);
static_assert(std::is_copy_constructible_v<string_identity>);
static_assert(std::is_move_constructible_v<string_identity>);
static_assert(std::is_copy_assignable_v<string_identity>);
static_assert(std::is_move_assignable_v<string_identity>);

// Converts back to a value or an rvalue, never to a mutable lvalue.
static_assert(std::is_convertible_v<const int_identity&, int>);
static_assert(std::is_convertible_v<string_identity&&, std::string&&>);
static_assert(!std::is_convertible_v<string_identity&, std::string&>);
static_assert(!std::is_convertible_v<const string_identity&, std::string&>);

// noexcept follows the wrapped type.
static_assert(std::is_nothrow_default_constructible_v<int_identity>);
static_assert(std::is_nothrow_copy_constructible_v<int_identity>);
static_assert(std::is_nothrow_move_constructible_v<string_identity>);
static_assert(std::is_nothrow_move_assignable_v<string_identity>);
static_assert(!std::is_nothrow_copy_constructible_v<string_identity>);
static_assert(!std::is_nothrow_copy_assignable_v<string_identity>);
static_assert(
    !std::is_nothrow_copy_constructible_v<const_identity<throwing_copy>>);
static_assert(
    std::is_nothrow_move_constructible_v<const_identity<throwing_copy>>);
static_assert(!std::is_nothrow_constructible_v<
              const_identity<throwing_copy>, const throwing_copy&>);
static_assert(std::is_nothrow_swappable_v<string_identity>);
static_assert(std::is_nothrow_destructible_v<string_identity>);

// `assign` accepts anything convertible to the wrapped type.
static_assert(can_assign<string_identity, const char*>);
static_assert(can_assign<string_identity, std::string>);
static_assert(can_assign<int_identity, long>);
static_assert(!can_assign<string_identity, std::vector<int>>);
static_assert(!can_assign<int_identity, std::string>);

constexpr bool usable_in_constexpr() {
    int_identity value{ 1 };
    int_identity copy = value;
    copy.assign(5);
    int_identity other{ 7 };
    copy.swap(other);
    int_identity moved{ std::move(other) };
    int_identity assigned;
    assigned = moved;
    assigned = int_identity{ 9 };

    return static_cast<int>(value) == 1 && static_cast<int>(copy) == 7 &&
           static_cast<int>(moved) == 5 && static_cast<int>(assigned) == 9;
}
static_assert(usable_in_constexpr());

void test_constructs_from_lvalue_and_rvalue_strings() {
    const std::string long_text(long_length, 'k');

    string_identity from_lvalue{ long_text };
    string_identity from_rvalue{ std::string(long_text) };

    require(text_of(from_lvalue) == long_text);
    require(text_of(from_rvalue) == long_text);
}

void test_lvalue_conversion_copies() {
    const std::string long_text(long_length, 'k');
    string_identity identity{ long_text };

    std::string copied = identity;

    require(copied == long_text);
    require(text_of(identity) == long_text);
}

void test_rvalue_conversion_yields_movable_reference() {
    const std::string long_text(long_length, 'k');
    string_identity identity{ long_text };

    std::string&& stolen = std::move(identity);
    std::string moved{ std::move(stolen) };

    require(moved == long_text);
}

void test_wrapper_copy_and_move_keep_the_value() {
    const std::string long_text(long_length, 'k');
    string_identity source{ long_text };

    string_identity copy{ source };
    string_identity move{ std::move(copy) };
    copy = move;

    require(text_of(copy) == long_text);
    require(text_of(move) == long_text);
}

void test_move_assignment_replaces_the_value() {
    const std::string long_text(long_length, 'k');
    string_identity source{ long_text };
    string_identity target{ std::string("short") };

    target = std::move(source);

    require(text_of(target) == long_text);
}

void test_swap_exchanges_values() {
    const std::string long_right(40, 'r');
    string_identity left{ std::string("left") };
    string_identity right{ long_right };

    left.swap(right);

    require(text_of(left) == long_right);
    require(text_of(right) == "left");
}

void test_assign_replaces_the_value() {
    const std::string long_text(50, 'x');
    const std::string source("from-lvalue");
    string_identity from_literal{ std::string("left") };
    string_identity from_rvalue{ std::string("right") };
    string_identity from_lvalue;

    from_literal.assign("literal");
    from_rvalue.assign(std::string(long_text));
    from_lvalue.assign(source);

    require(text_of(from_literal) == "literal");
    require(text_of(from_rvalue) == long_text);
    require(text_of(from_lvalue) == source);
}

void test_works_as_pair_key_in_vector() {
    // The intended use as a logically const key inside a pair.
    const std::string long_key(32, 't');
    std::vector<std::pair<string_identity, int>> entries;
    entries.emplace_back(std::string("one"), 1);
    entries.emplace_back(std::string("two"), 2);
    entries.emplace_back(long_key, 3);

    entries.erase(entries.begin());
    entries.front().first.assign("changed");

    require_or_return(entries.size() == 2, );
    require(text_of(entries[0].first) == "changed");
    require(entries[0].second == 2);
    require(text_of(entries[1].first) == long_key);
}

} // namespace

int main() {
    test_constructs_from_lvalue_and_rvalue_strings();
    test_lvalue_conversion_copies();
    test_rvalue_conversion_yields_movable_reference();
    test_wrapper_copy_and_move_keep_the_value();
    test_move_assignment_replaces_the_value();
    test_swap_exchanges_values();
    test_assign_replaces_the_value();
    test_works_as_pair_key_in_vector();
    return neutron_test::result();
}
