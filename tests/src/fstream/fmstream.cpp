#include <cstddef>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/fstream.hpp>
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

/// A window size of 1 is rounded up to one page, so these files span several
/// windows.
constexpr std::size_t tiny_window = 1;

constexpr std::size_t long_size = 10000;

constexpr std::size_t alphabet_size = 26;

constexpr auto in_out_trunc = std::ios::in | std::ios::out | std::ios::trunc;

// An input and output stream, but not a std::fstream.
static_assert(std::is_base_of_v<std::iostream, fmstream>);
static_assert(!std::is_base_of_v<std::fstream, fmstream>);
static_assert(std::is_base_of_v<std::wiostream, wfmstream>);
static_assert(std::is_nothrow_move_constructible_v<fmstream>);

/// Multiples of the alphabet, so that misaligned content is obvious.
std::string make_text(std::size_t size) {
    std::string text(size, 'a');
    for (std::size_t i = 0; i != size; ++i) {
        text[i] = static_cast<char>('a' + (i % alphabet_size));
    }
    return text;
}

std::string read_all(const temp_file& file) {
    std::ifstream stream(file.path(), std::ios::binary);
    return { std::istreambuf_iterator<char>(stream),
             std::istreambuf_iterator<char>() };
}

void write_all(const temp_file& file, const std::string& text) {
    std::ofstream stream(file.path(), std::ios::binary);
    stream << text;
}

void test_bytes_survive_a_round_trip() {
    const temp_file file("fms");
    const auto text = make_text(long_size);
    {
        fmstream stream(file.path(), in_out_trunc, tiny_window);
        stream << text;
    }

    require(read_all(file) == text);
}

void test_written_bytes_read_back_in_the_same_stream() {
    const temp_file file("fms");
    const auto text = make_text(long_size);
    fmstream stream(file.path(), in_out_trunc, tiny_window);
    stream << text;
    stream.seekg(0);
    std::string copy(text.size(), '\0');

    stream.read(copy.data(), static_cast<std::streamsize>(copy.size()));

    require(stream.gcount() == static_cast<std::streamsize>(text.size()));
    require(copy == text);
    require(stream.peek() == fmstream::traits_type::eof());
}

void test_reading_stops_at_the_written_size() {
    const temp_file file("fms");
    fmstream stream(file.path(), in_out_trunc, tiny_window);
    stream << "abc";
    stream.seekg(1);
    std::string copy(8, '\0');

    stream.read(copy.data(), static_cast<std::streamsize>(copy.size()));

    // The file grew by a window, but only three bytes are content.
    require(stream.gcount() == 2);
}

void test_read_and_write_share_the_position() {
    const temp_file file("fms");
    fmstream stream(file.path(), in_out_trunc, tiny_window);
    stream << "0123456789";
    stream.seekg(2);

    require(stream.get() == '2');
    require(stream.tellp() == std::streampos(3));
    stream.put('x');
    require(stream.tellg() == std::streampos(4));
    require(stream.peek() == '4');
    stream.seekg(-1, std::ios::cur);
    require(stream.get() == 'x');
}

void test_seeking_past_the_end_and_writing_extends_the_content() {
    constexpr std::streamoff past_end = 5;
    const temp_file file("fms");
    {
        fmstream stream(file.path(), in_out_trunc, tiny_window);
        stream << "ab";
        stream.seekp(past_end);
        stream << 'z';
        stream.seekg(0, std::ios::end);

        require(stream.tellg() == std::streampos(6));
    }

    require(read_all(file) == std::string("ab\0\0\0z", 6));
}

void test_in_out_keeps_an_existing_file() {
    const temp_file file("fms");
    write_all(file, "abcdef");
    {
        fmstream stream(file.path(), std::ios::in | std::ios::out, tiny_window);
        stream << "XY";
    }

    require(read_all(file) == "XYcdef");
}

void test_in_out_requires_an_existing_file() {
    const temp_file file("fms");

    const fmstream stream(file.path());

    require_false(stream.is_open());
    require(stream.fail());
}

void test_in_app_reads_from_the_start_and_writes_at_the_end() {
    const temp_file file("fms");
    write_all(file, "abc");
    {
        fmstream stream(file.path(), std::ios::in | std::ios::app, tiny_window);
        require(stream.get() == 'a');

        stream << "def";
        stream.seekg(1);

        require(stream.get() == 'b');
    }

    require(read_all(file) == "abcdef");
}

void test_overwriting_keeps_the_size() {
    const temp_file file("fms");
    {
        fmstream stream(file.path(), in_out_trunc, tiny_window);
        stream << "0123456789";
        stream.seekp(4);
        stream << "zz";
    }

    require(read_all(file) == "0123zz6789");
}

void test_formatted_round_trip() {
    const temp_file file("fms");
    fmstream stream(file.path(), in_out_trunc);
    stream << 4 << ' ' << "neutron";
    stream.seekg(0);
    int value = 0;
    std::string word;

    stream >> value >> word;

    require(value == 4);
    require(word == "neutron");
}

void test_flush_exposes_the_exact_content() {
    const temp_file file("fms");
    fmstream stream(file.path(), in_out_trunc, tiny_window);
    stream << "neutron";

    stream.flush();

    require(read_all(file) == "neutron");
}

void test_move_keeps_the_position() {
    const temp_file file("fms");
    fmstream stream(file.path(), in_out_trunc, tiny_window);
    stream << "abc";

    fmstream moved(std::move(stream));
    moved << "def";

    require(moved.tellp() == std::streampos(6));
    require_false(stream.is_open()); // NOLINT(bugprone-use-after-move)
}

void test_app_and_trunc_together_set_failbit() {
    const temp_file file("fms");

    const fmstream stream(file.path(), std::ios::app | std::ios::trunc);

    require(stream.fail());
}

} // namespace

int main() {
    test_bytes_survive_a_round_trip();
    test_written_bytes_read_back_in_the_same_stream();
    test_reading_stops_at_the_written_size();
    test_read_and_write_share_the_position();
    test_seeking_past_the_end_and_writing_extends_the_content();
    test_in_out_keeps_an_existing_file();
    test_in_out_requires_an_existing_file();
    test_in_app_reads_from_the_start_and_writes_at_the_end();
    test_overwriting_keeps_the_size();
    test_formatted_round_trip();
    test_flush_exposes_the_exact_content();
    test_move_keeps_the_position();
    test_app_and_trunc_together_set_failbit();
    return neutron_test::result();
}
