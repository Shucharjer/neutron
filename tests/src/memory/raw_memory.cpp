#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

constexpr std::size_t array_size = 32;
/// Number of leading elements written by the `_n` and partial range calls.
constexpr std::size_t prefix = 4;

using array_t = std::array<char, array_size>;

/// A zero padded string used as the copy source.
constexpr array_t source = [] {
    const std::string string = "this is a simple string";
    array_t array{};
    std::ranges::copy(string, array.begin());
    return array;
}();

// Every algorithm is usable in constant evaluation.

// uninitialized_copy copies the whole range.
constexpr array_t copied = [] {
    array_t result;
    uninitialized_copy(source.begin(), source.end(), result.begin());
    return result;
}();
static_assert(copied == source);

// uninitialized_copy_n copies only the first `prefix` elements.
constexpr array_t copied_n = [] {
    array_t result{};
    uninitialized_copy_n(source.begin(), prefix, result.begin());
    return result;
}();
static_assert(copied_n[0] == 't');
static_assert(copied_n[1] == 'h');
static_assert(copied_n[2] == 'i');
static_assert(copied_n[3] == 's');
static_assert(copied_n[prefix] == '\0');

// uninitialized_fill fills only the given range.
constexpr array_t filled = [] {
    array_t result{};
    uninitialized_fill(result.begin(), result.begin() + prefix, '4');
    return result;
}();
static_assert(filled[0] == '4');
static_assert(filled[1] == '4');
static_assert(filled[2] == '4');
static_assert(filled[3] == '4');
static_assert(filled[prefix] == '\0');

// uninitialized_fill_n fills only the first `prefix` elements.
constexpr array_t filled_n = [] {
    array_t result{};
    uninitialized_fill_n(result.begin(), prefix, '4');
    return result;
}();
static_assert(filled_n[0] == '4');
static_assert(filled_n[1] == '4');
static_assert(filled_n[2] == '4');
static_assert(filled_n[3] == '4');
static_assert(filled_n[prefix] == '\0');

// Default and value construction zero the elements in constant evaluation.
constexpr array_t default_constructed = [] {
    array_t result;
    uninitialized_default_construct(result.begin(), result.end());
    return result;
}();
static_assert(default_constructed == array_t{});

constexpr array_t value_constructed = [] {
    array_t result;
    uninitialized_value_construct(result.begin(), result.end());
    return result;
}();
static_assert(value_constructed == array_t{});

constexpr array_t value_constructed_n = [] {
    array_t result;
    uninitialized_value_construct_n(result.begin(), array_size);
    return result;
}();
static_assert(value_constructed_n == array_t{});

} // namespace

int main() { return neutron_test::result(); }
