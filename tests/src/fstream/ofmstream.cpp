#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/fstream.hpp>
#include "neutron/detail/utility/macros.hpp"
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

/// A window size of 1 is rounded up to the mapping granularity, so these
/// files span several windows.
constexpr std::size_t tiny_window = 1;

/// Two and a half windows; the granularity is a 4 KiB page on POSIX but 64 KiB
/// on Windows.
const std::size_t long_size = internal::file_mapping::granularity() * 5 / 2;

constexpr std::size_t alphabet_size = 26;

// An output stream, but not a std::ofstream.
static_assert(std::is_base_of_v<std::ostream, ofmstream>);
static_assert(!std::is_base_of_v<std::ofstream, ofmstream>);
static_assert(std::is_base_of_v<std::wostream, wofmstream>);
static_assert(std::is_nothrow_move_constructible_v<ofmstream>);
static_assert(!std::is_copy_constructible_v<ofmstream>);

std::string read_all(const temp_file& file) {
    std::ifstream stream(file.path(), std::ios::binary);
    return { std::istreambuf_iterator<char>(stream),
             std::istreambuf_iterator<char>() };
}

/// Multiples of the alphabet, so that misaligned content is obvious.
std::string make_text(std::size_t size) {
    std::string text(size, 'a');
    for (std::size_t i = 0; i != size; ++i) {
        text[i] = static_cast<char>('a' + (i % alphabet_size));
    }
    return text;
}

void test_out_creates_the_file() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.string());
        require(stream.is_open());

        stream << "neutron " << 2 << '\n';
    }

    require(read_all(file) == "neutron 2\n");
}

void test_out_truncates_an_existing_file() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path());
        stream << "abcdef";
    }
    {
        const ofmstream stream(file.path());
        require(stream.is_open());
    }

    require(read_all(file).empty());
}

void test_long_text_spans_windows() {
    const temp_file file("ofs");
    const auto text = make_text(long_size);
    {
        ofmstream stream(file.path(), std::ios::out, tiny_window);
        stream << text;

        require(stream.tellp() == std::streampos(long_size));
    }

    require(read_all(file) == text);
}

void test_close_keeps_every_written_byte() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path(), std::ios::out, tiny_window);
        stream << "0123456789";
        stream.seekp(3);
        stream << 'x';
    }

    require(read_all(file) == "012x456789");
}

void test_writing_past_the_end_fills_with_zeros() {
    constexpr std::size_t gap = 100;
    const temp_file file("ofs");
    {
        ofmstream stream(file.path(), std::ios::out, tiny_window);
        stream.seekp(gap);
        stream.put('z');
    }

    const auto text = read_all(file);

    require(text.size() == gap + 1);
    require(text[gap] == 'z');
    require(text[0] == '\0');
    require(text[gap - 1] == '\0');
}

void test_app_writes_at_the_end_after_seeking() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path());
        stream << "abc";
    }
    {
        ofmstream stream(file.path(), std::ios::app, tiny_window);
        stream << "def";
        stream.seekp(0);
        stream << 'X';
    }

    require(read_all(file) == "abcdefX");
}

void test_app_keeps_an_existing_file() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path());
        stream << "kept";
    }
    { const ofmstream stream(file.path(), std::ios::app); }

    require(read_all(file) == "kept");
}

void test_ate_starts_at_the_end() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path());
        stream << "1234";
    }
    ofmstream stream(file.path(), std::ios::in | std::ios::ate);

    require(stream.tellp() == std::streampos(4));
}

void test_flush_makes_the_size_exact() {
    const temp_file file("ofs");
    ofmstream stream(file.path(), std::ios::out, tiny_window);
    stream << "neutron";

    stream.flush();

    require(stream.good());
    require(read_all(file) == "neutron");
}

void test_binary_values_round_trip() {
    constexpr std::uint32_t value = 0x01020304;
    const temp_file file("ofs");
    {
        ofmstream stream(file.path(), std::ios::binary);
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    const auto text = read_all(file);

    require(text.size() == sizeof(value));
    require(std::memcmp(text.data(), &value, sizeof(value)) == 0);
}

void test_an_unwritable_path_sets_failbit() {
    const temp_file file("ofs");

    const ofmstream stream(file.path() / "missing" / "file");

    require_false(stream.is_open());
    require(stream.fail());
}

void test_failure_throws_when_requested() {
    const temp_file file("ofs");
    ofmstream stream;
    stream.exceptions(std::ios::failbit);
    bool thrown = false;

    ATOM_TRY { stream.open(file.path() / "missing" / "file"); }
    ATOM_CATCH(const std::ios_base::failure&) { thrown = true; }

    require(thrown);
}

void test_open_clears_the_state() {
    const temp_file file("ofs");
    ofmstream stream(file.path() / "missing" / "file");

    stream.open(file.path());

    require(stream.good());
}

void test_closing_twice_sets_failbit() {
    const temp_file file("ofs");
    ofmstream stream(file.path());
    stream.close();
    require(stream.good());

    stream.close();

    require(stream.fail());
}

void test_writing_without_a_file_sets_badbit() {
    ofmstream stream;

    stream << 'x';

    require(stream.bad());
}

void test_move_transfers_the_file() {
    const temp_file file("ofs");
    {
        ofmstream stream(file.path());
        stream << "ab";

        ofmstream moved(std::move(stream));
        moved << "cd";

        require(moved.tellp() == std::streampos(4));
        require_false(stream.is_open()); // NOLINT(bugprone-use-after-move)
    }

    require(read_all(file) == "abcd");
}

void test_move_assignment_closes_the_previous_file() {
    const temp_file first("ofs");
    const temp_file second("ofs");
    ofmstream stream(first.path(), std::ios::out, tiny_window);
    stream << "first";
    ofmstream other(second.path());
    other << "second";

    stream = std::move(other);

    require(read_all(first) == "first");
    require(stream.tellp() == std::streampos(6));
}

} // namespace

int main() {
    test_out_creates_the_file();
    test_out_truncates_an_existing_file();
    test_long_text_spans_windows();
    test_close_keeps_every_written_byte();
    test_writing_past_the_end_fills_with_zeros();
    test_app_writes_at_the_end_after_seeking();
    test_app_keeps_an_existing_file();
    test_ate_starts_at_the_end();
    test_flush_makes_the_size_exact();
    test_binary_values_round_trip();
    test_an_unwritable_path_sets_failbit();
    test_failure_throws_when_requested();
    test_open_clears_the_state();
    test_closing_twice_sets_failbit();
    test_writing_without_a_file_sets_badbit();
    test_move_transfers_the_file();
    test_move_assignment_closes_the_previous_file();
    return neutron_test::result();
}
