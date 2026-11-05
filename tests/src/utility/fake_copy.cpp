#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Counts copy constructions.
struct counted {
    static inline int copies = 0;
    counted()                = default;
    counted(const counted&) { ++copies; }
    counted(counted&&) noexcept            = default;
    counted& operator=(const counted&)     = default;
    counted& operator=(counted&&) noexcept = default;
    ~counted()                             = default;
};

void function(int) {}

/// Result type of `fake_copy` applied to an expression of type `T`.
template <typename T>
using fake_copy_result = decltype(fake_copy(std::declval<T>()));

// The result is a pointer to the decayed copy type.
static_assert(std::is_same_v<decltype(fake_copy(1)), int*>);
static_assert(std::is_same_v<fake_copy_result<int&>, int*>);
static_assert(std::is_same_v<fake_copy_result<const int&>, int*>);
static_assert(std::is_same_v<fake_copy_result<volatile int&>, int*>);
static_assert(
    std::is_same_v<fake_copy_result<const std::string&>, std::string*>);
static_assert(std::is_same_v<fake_copy_result<std::string&&>, std::string*>);
static_assert(std::is_same_v<
              std::remove_pointer_t<fake_copy_result<std::shared_ptr<int>&>>,
              std::shared_ptr<int>>);

// Arrays and functions decay.
static_assert(std::is_same_v<fake_copy_result<int (&)[3]>, int**>);
static_assert(
    std::is_same_v<fake_copy_result<const char (&)[4]>, const char**>);
static_assert(std::is_same_v<decltype(fake_copy(function)), void (**)(int)>);

// Pointers keep their pointee qualifiers.
static_assert(std::is_same_v<fake_copy_result<const int* const&>, const int**>);

// The returned pointer is null, also in constant evaluation.
static_assert(fake_copy(1) == nullptr);
static_assert(fake_copy(std::string("constexpr")) == nullptr);

void test_returns_null_pointers() {
    counted value;
    std::string text(64, 's');

    auto* lvalue   = fake_copy(value);
    auto* rvalue   = fake_copy(counted{});
    auto* text_ptr = fake_copy(text);

    static_assert(std::is_same_v<decltype(lvalue), counted*>);
    require(lvalue == nullptr);
    require(rvalue == nullptr);
    require(text_ptr == nullptr);
}

void test_evaluated_call_copies_its_argument() {
    counted value;
    counted::copies = 0;

    static_cast<void>(fake_copy(value));
    static_cast<void>(fake_copy(counted{}));

    // The named parameter is copied, even for rvalues.
    require(counted::copies == 2);
}

void test_evaluated_call_leaves_the_source_intact() {
    const std::string expected(64, 's');
    std::string text = expected;

    static_cast<void>(fake_copy(text));

    require(text == expected);
}

void test_unevaluated_call_does_not_copy() {
    counted value;
    counted::copies = 0;

    using type = std::remove_pointer_t<decltype(fake_copy(value))>;
    static_assert(std::is_same_v<type, counted>);
    function(0);

    require(counted::copies == 0);
}

} // namespace

int main() {
    test_returns_null_pointers();
    test_evaluated_call_copies_its_argument();
    test_evaluated_call_leaves_the_source_intact();
    test_unevaluated_call_does_not_copy();
    return neutron_test::result();
}
