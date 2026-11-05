#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <neutron/string.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

struct convertible {
    operator std::string_view() const noexcept { return "convertible"; }
};

struct explicit_only {
    explicit operator std::string_view() const noexcept { return "explicit"; }
};

// Anything implicitly convertible to std::string_view qualifies.
static_assert(string_view_like<std::string_view>);
static_assert(string_view_like<const std::string_view&>);
static_assert(string_view_like<std::string>);
static_assert(string_view_like<const std::string&>);
static_assert(string_view_like<const char*>);
static_assert(string_view_like<char*>);
static_assert(string_view_like<const char (&)[4]>);
static_assert(string_view_like<convertible>);

// Non-strings, other character types and explicit conversions do not.
static_assert(!string_view_like<int>);
static_assert(!string_view_like<char>);
static_assert(!string_view_like<std::vector<char>>);
static_assert(!string_view_like<const wchar_t*>);
static_assert(!string_view_like<std::wstring>);
static_assert(!string_view_like<explicit_only>);
static_assert(!string_view_like<void>);

template <string_view_like Str>
std::size_t length_of(const Str& str) {
    return std::string_view{ str }.size();
}

void test_constrained_function_accepts_string_like_arguments() {
    const std::string str = "abcd";

    require(length_of("abc") == 3);
    require(length_of(str) == 4);
    require(length_of(std::string_view{ "ab" }) == 2);
    require(length_of(convertible{}) == 11);
}

} // namespace

int main() {
    test_constrained_function_accepts_string_like_arguments();
    return neutron_test::result();
}
