#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <cstring>
    #include <format>
    #include <string_view>
    #include <type_traits>
    #include <neutron/string.hpp>
#endif
#include "require.hpp"

using namespace neutron;

namespace {

static_assert(std::is_trivially_copyable_v<inplace_string<8>>);
static_assert(inplace_string<8>::capacity() == 8);
static_assert(inplace_string<4>{ "abc" }.view() == "abc");
static_assert(!inplace_string<4>{ "abc" }.truncated());
static_assert(inplace_string<3>{ "abcdef" }.view() == "abc");
static_assert(inplace_string<3>{ "abcdef" }.truncated());
static_assert(inplace_string<4>{}.empty());

void test_assign() {
    inplace_string<5> str{ std::string_view{ "hello world" } };
    require(str.view() == "hello");
    require(str.truncated());
    str.assign("hi");
    require(str.view() == "hi");
    require(!str.truncated());
    require(str.size() == 2);
    require(str == inplace_string<5>{ "hi" });
}

void test_copy_as_bytes() {
    const inplace_string<16> source{ "copied" };
    alignas(inplace_string<16>) std::byte buffer[sizeof(source)];
    std::memcpy(buffer, &source, sizeof(source));
    inplace_string<16> target;
    std::memcpy(&target, buffer, sizeof(target));
    require(target.view() == "copied");
}

void test_format() {
    require(std::format("[{}]", inplace_string<8>{ "abc" }) == "[abc]");
    require(std::format("[{:>5}]", inplace_string<8>{ "ab" }) == "[   ab]");
    require(std::format("[{}]", inplace_string<2>{ "abc" }) == "[ab]");
}

} // namespace

int main() {
    test_assign();
    test_copy_as_bytes();
    test_format();
    return neutron_test::result();
}
