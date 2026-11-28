#include "neutron/detail/utility/macros.hpp"
#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <array>
    #include <cstddef>
    #include <format>
    #include <ios>
    #include <iostream>
    #include <memory>
    #include <sstream>
    #include <stdexcept>
    #include <string>
    #include <string_view>
    #include <neutron/string.hpp>
#endif
#include "require.hpp"

using namespace neutron;

namespace {

/// Inline capacity small enough that ordinary words spill to the heap.
constexpr std::size_t tiny_capacity = 4;

/// Allocator whose allocation always fails, to force exceptions mid-read.
template <typename T>
struct throwing_allocator {
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    T* allocate(std::size_t /*n*/) { throw std::length_error("no memory"); }

    void deallocate(T* /*ptr*/, std::size_t /*n*/) const noexcept {}
};

using throwing_string = basic_short_string<
    tiny_capacity, char, std::char_traits<char>, throwing_allocator<char>>;

template <std::size_t Size>
constexpr void default_construct() {
    using string_t = short_string<Size>;

    string_t string;
}

// A literal that fills the whole capacity is accepted in constant evaluation.
[[maybe_unused]] constexpr auto full_capacity_literal = [] {
    short_string<16> str = "this is a string";
    std::array<char, 32> r{};

    return r;
}();

void test_default_constructible_for_every_capacity() {
    default_construct<0>();
    default_construct<8>();
    default_construct<16>();
    default_construct<32>();
    default_construct<64>();
}

void test_formatter_prints_the_contents() {
    short_string ss = "a short string";

    const auto text = std::format("{}", ss);

    std::cout << text;
    require(text == "a short string");
}

void test_insertion_writes_the_contents() {
    const short_string<16> ss = "neutron";
    std::ostringstream os;

    os << ss;

    require(os.str() == "neutron");
}

void test_extraction_reads_one_word() {
    std::istringstream is("hello world");
    short_string<16> ss;

    is >> ss;

    require(std::string_view(ss) == "hello");
}

void test_extraction_skips_leading_whitespace() {
    std::istringstream is(" \t\n word");
    short_string<16> ss;

    is >> ss;

    require(std::string_view(ss) == "word");
}

void test_extraction_leaves_the_delimiter_in_the_stream() {
    std::istringstream is("word\nrest");
    short_string<16> ss;

    is >> ss;

    require(is.peek() == '\n');
}

void test_extraction_reads_consecutive_words() {
    std::istringstream is("first second");
    short_string<16> first;
    short_string<16> second;

    is >> first >> second;

    require(std::string_view(first) == "first");
    require(std::string_view(second) == "second");
}

void test_extraction_replaces_previous_contents() {
    std::istringstream is("new");
    short_string<16> ss = "old contents";

    is >> ss;

    require(std::string_view(ss) == "new");
}

void test_extraction_keeps_the_string_null_terminated() {
    std::istringstream is("abc");
    short_string<16> ss = "a longer word";

    is >> ss;

    require(ss.c_str()[ss.size()] == '\0');
}

void test_extraction_stops_after_width_characters() {
    std::istringstream is("abcdef");
    short_string<16> ss;
    constexpr std::streamsize width = 3;

    is.width(width);
    is >> ss;

    require(std::string_view(ss) == "abc");
    require(is.peek() == 'd');
}

void test_extraction_resets_width() {
    std::istringstream is("abcdef");
    short_string<16> ss;
    constexpr std::streamsize width = 3;

    is.width(width);
    is >> ss;

    require(is.width() == 0);
}

void test_extraction_grows_beyond_inline_capacity() {
    constexpr std::string_view word = "a_word_longer_than_the_buffer";
    std::istringstream is{ std::string{ word } };
    short_string<tiny_capacity> ss;

    is >> ss;

    require(std::string_view(ss) == word);
    require(ss.capacity() > word.size());
    require(ss.c_str()[ss.size()] == '\0');
}

void test_extraction_into_zero_capacity_allocates() {
    std::istringstream is("zero");
    short_string<0> ss;

    is >> ss;

    require(std::string_view(ss) == "zero");
}

void test_extraction_matches_std_string() {
    constexpr std::string_view input = "  alpha\tbeta_is_long \n gamma  ";
    std::istringstream expected_is{ std::string{ input } };
    std::istringstream actual_is{ std::string{ input } };
    std::string expected;
    short_string<tiny_capacity> actual;

    while (expected_is >> expected) {
        actual_is >> actual;
        require(std::string_view(actual) == expected);
    }
    actual_is >> actual;

    require(actual_is.rdstate() == expected_is.rdstate());
}

void test_extraction_at_end_of_input_sets_eofbit() {
    std::istringstream is("last");
    short_string<16> ss;

    is >> ss;

    require(is.eof());
    require_false(is.fail());
}

void test_extraction_from_blank_input_sets_failbit() {
    std::istringstream is("   ");
    short_string<16> ss;

    is >> ss;

    require(is.fail());
}

void test_extraction_from_failed_stream_keeps_contents() {
    std::istringstream is("word");
    short_string<16> ss = "kept";

    is.setstate(std::ios_base::failbit);
    is >> ss;

    require(std::string_view(ss) == "kept");
}

void test_extraction_reads_wide_characters() {
    std::wistringstream is(L"  wide string");
    short_wstring<16> ss;

    is >> ss;

    require(std::wstring_view(ss) == L"wide");
}

void test_extraction_failure_sets_badbit() {
    std::istringstream is("long_enough_to_allocate");
    throwing_string ss;

    is >> ss;

    require(is.bad());
}

void test_extraction_failure_keeps_unread_characters() {
    // The fourth character needs a heap buffer, which cannot be allocated.
    std::istringstream is("abcdef");
    throwing_string ss;

    is >> ss;
    is.clear();

    require(is.peek() == 'd');
}

void test_extraction_failure_rethrows_the_original_exception() {
    std::istringstream is("long_enough_to_allocate");
    throwing_string ss;
    bool rethrown = false;

    is.exceptions(std::ios_base::badbit);
    ATOM_TRY { is >> ss; }
    ATOM_CATCH(const std::length_error&) { rethrown = true; }

    require(rethrown);
    require(is.bad());
}

} // namespace

int main() {
    test_default_constructible_for_every_capacity();
    test_formatter_prints_the_contents();
    test_insertion_writes_the_contents();
    test_extraction_reads_one_word();
    test_extraction_skips_leading_whitespace();
    test_extraction_leaves_the_delimiter_in_the_stream();
    test_extraction_reads_consecutive_words();
    test_extraction_replaces_previous_contents();
    test_extraction_keeps_the_string_null_terminated();
    test_extraction_stops_after_width_characters();
    test_extraction_resets_width();
    test_extraction_grows_beyond_inline_capacity();
    test_extraction_into_zero_capacity_allocates();
    test_extraction_matches_std_string();
    test_extraction_at_end_of_input_sets_eofbit();
    test_extraction_from_blank_input_sets_failbit();
    test_extraction_from_failed_stream_keeps_contents();
    test_extraction_reads_wide_characters();
    test_extraction_failure_sets_badbit();
    test_extraction_failure_keeps_unread_characters();
    test_extraction_failure_rethrows_the_original_exception();
    return neutron_test::result();
}
