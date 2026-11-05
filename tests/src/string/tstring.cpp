#include <cstddef>
#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <neutron/string.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

template <tstring Str>
struct tag {
    static constexpr auto value = Str;
};

template <tstring Str>
constexpr std::size_t size_of() noexcept {
    return Str.size();
}

// Deduction from literals: the extent includes the terminator.
static_assert(std::is_same_v<decltype(tstring{ "abc" }), tstring<4>>);
static_assert(std::is_same_v<decltype(tstring{ "" }), tstring<1>>);

// Structural type usable as a non-type template parameter.
static_assert(std::is_same_v<tag<"abc">, tag<"abc">>);
static_assert(!std::is_same_v<tag<"abc">, tag<"abd">>);
static_assert(!std::is_same_v<tag<"abc">, tag<"ab">>);
static_assert(tag<"neutron">::value.view() == "neutron");
static_assert(size_of<"hello">() == 5);
static_assert(size_of<"">() == 0);

// Compile-time observers and same-length comparison.
static_assert(tstring{ "abc" }.size() == 3);
static_assert(tstring{ "abc" }.length() == 3);
static_assert(tstring{ "abc" }.capacity() == 4);
static_assert(!tstring{ "abc" }.empty());
static_assert(tstring{ "" }.empty());
static_assert(tstring{ "" }.size() == 0);
static_assert(tstring{ "abc" }.view() == "abc");
static_assert(tstring{ "abc" }.data()[3] == '\0');
static_assert(tstring{ "abc" } == tstring{ "abc" });
static_assert(!(tstring{ "abc" } == tstring{ "abd" }));
static_assert(tstring{ "abc" } != tstring{ "abd" });
static_assert(!(tstring{ "abc" } != tstring{ "abc" }));
static_assert(!(tstring{ "ab" } == tstring{ "abc" }));
static_assert(!(tstring{ "abc" } == tstring{ "ab" }));

constexpr bool constexpr_copy() {
    tstring str{ "xyz" };
    auto copy     = str;
    copy.value[0] = 'a';
    return str.view() == "xyz" && copy.view() == "ayz" && copy != str;
}
static_assert(constexpr_copy());

void test_construct_from_runtime_array() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    const char literal[]         = "runtime";
    constexpr std::size_t length = 7;

    const tstring str{ literal };

    require(str.size() == length);
    require(str.length() == length);
    require(str.capacity() == length + 1);
    require_false(str.empty());
    require(std::strcmp(str.c_str(), "runtime") == 0);
    require(str.data() == str.c_str());
    require(str.data() == str.value);
    require(str.view() == "runtime");
    require(str.view().size() == str.size());
}

void test_equality_is_symmetric() {
    const tstring abc{ "abc" };
    const tstring abc2{ "abc" };
    const tstring abd{ "abd" };
    const tstring ab{ "ab" };

    require(abc == abc2);
    require(abc2 == abc);
    require_false(abc != abc2);
    require_false(abc == abd);
    require_false(abd == abc);
    require(abc != abd);
    require(abd != abc);
    require_false(abc == ab);
    require_false(ab == abc);
}

void test_same_length_ordering() {
    const tstring abc{ "abc" };
    const tstring abd{ "abd" };

    require(abc < abd);
    require(abd > abc);
    require(abc <= abd);
    require(abd >= abc);
    require(abc <= abc);
    require(abc >= abc);
    require_false(abc < abc);
    require((abc <=> abc) == 0);
    require((abc <=> abd) < 0);
    require((abd <=> abc) > 0);
}

void test_different_length_ordering_is_lexicographic() {
    const tstring abc{ "abc" };
    const tstring ab{ "ab" };
    const tstring bb{ "b" };
    const tstring empty{ "" };

    // A prefix orders before the longer string.
    require(ab < abc);
    require(abc > ab);
    require((ab <=> abc) < 0);
    require((abc <=> ab) > 0);
    require(empty < ab);
    require(ab > empty);

    // The first differing character decides, not the length.
    require(abc < bb);
    require(bb > abc);
    require((bb <=> abc) > 0);
}

void test_format_honours_width_and_alignment() {
    require(std::format("{}", tstring{ "abc" }) == "abc");
    require(std::format("[{:>5}]", tstring{ "abc" }) == "[  abc]");
    require(std::format("[{:<4}]", tstring{ "" }) == "[    ]");
    require(std::format("{}", tag<"tag">::value) == "tag");
}

} // namespace

int main() {
    test_construct_from_runtime_array();
    test_equality_is_symmetric();
    test_same_length_ordering();
    test_different_length_ordering_is_lexicographic();
    test_format_honours_width_and_alignment();
    return neutron_test::result();
}
