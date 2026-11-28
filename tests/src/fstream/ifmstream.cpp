#include "neutron/detail/utility/macros.hpp"
// With modules, an implementation unit of neutron that imports std, see
// `internal_tests`.
#ifndef ATOM_USES_MODULES
    #include <cstddef>
    #include <filesystem>
    #include <fstream>
    #include <ios>
    #include <istream>
    #include <sstream>
    #include <string>
    #include <type_traits>
    #include <utility>
    #include <neutron/fstream.hpp>
#endif
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

/// A window size of 1 is rounded up to the mapping granularity, so these
/// files span several windows.
constexpr std::size_t tiny_window = 1;

// An input stream, but not a std::ifstream.
static_assert(std::is_base_of_v<std::istream, ifmstream>);
static_assert(!std::is_base_of_v<std::ifstream, ifmstream>);
static_assert(std::is_base_of_v<std::wistream, wifmstream>);
static_assert(std::is_nothrow_move_constructible_v<ifmstream>);

void write_all(const temp_file& file, const std::string& text) {
    std::ofstream stream(file.path(), std::ios::binary);
    stream << text;
}

constexpr std::size_t max_line_length = 96;

constexpr std::size_t alphabet_size = 26;

/// Enough lines to span several windows; the granularity is a 4 KiB page on
/// POSIX but 64 KiB on Windows.
const std::size_t line_count = [] {
    constexpr std::size_t windows     = 3;
    constexpr std::size_t cycle_lines = max_line_length + 1;
    // Every length from 0 to max_line_length, each followed by a newline.
    constexpr std::size_t cycle_bytes = cycle_lines * (max_line_length + 2) / 2;
    const std::size_t bytes = windows * internal::file_mapping::granularity();
    return (bytes + cycle_bytes - 1) / cycle_bytes * cycle_lines;
}();

/// Line `index` of `make_lines`, varying in length and letter.
std::string line_of(std::size_t index) {
    // Braces would select the initializer-list constructor.
    return std::string( // NOLINT(modernize-return-braced-init-list)
        index % (max_line_length + 1),
        static_cast<char>('a' + (index % alphabet_size)));
}

/// Lines long enough to cross several windows.
std::string make_lines(std::size_t count) {
    std::string text;
    for (std::size_t i = 0; i != count; ++i) {
        text.append(line_of(i));
        text.push_back('\n');
    }
    return text;
}

void test_getline_crosses_windows() {
    const temp_file file("ifs");
    write_all(file, make_lines(line_count));
    ifmstream stream(file.path(), std::ios::in, tiny_window);
    std::string line;
    std::size_t lines = 0;
    bool intact       = true;

    while (std::getline(stream, line)) {
        intact = intact && line == line_of(lines);
        ++lines;
    }

    require(lines == line_count);
    require(intact);
    require(stream.eof());
}

void test_formatted_input_parses_numbers() {
    const temp_file file("ifs");
    write_all(file, "12 -7 word");
    ifmstream stream(file.string());
    int first  = 0;
    int second = 0;
    std::string word;

    stream >> first >> second >> word;

    require(first == 12);
    require(second == -7);
    require(word == "word");
}

void test_get_and_peek() {
    const temp_file file("ifs");
    write_all(file, "ab");
    ifmstream stream(file.path());

    require(stream.peek() == 'a');
    require(stream.tellg() == std::streampos(0));
    require(stream.get() == 'a');
    require(stream.get() == 'b');
    require(stream.get() == ifmstream::traits_type::eof());
    require(stream.eof());
}

void test_read_stops_at_the_end() {
    const temp_file file("ifs");
    write_all(file, "abc");
    ifmstream stream(file.path(), std::ios::in, tiny_window);
    std::string text(8, '\0');

    stream.read(text.data(), static_cast<std::streamsize>(text.size()));

    require(stream.gcount() == 3);
    require(text.substr(0, 3) == "abc");
    require(stream.eof());
    require(stream.fail());
}

void test_rdbuf_copies_the_whole_file() {
    const temp_file file("ifs");
    const auto text = make_lines(line_count);
    write_all(file, text);
    ifmstream stream(file.path(), std::ios::in, tiny_window);
    std::ostringstream copy;

    copy << stream.rdbuf();

    require(copy.str() == text);
}

void test_unget_crosses_windows() {
    const temp_file file("ifs");
    const auto text = make_lines(line_count);
    write_all(file, text);
    ifmstream stream(file.path(), std::ios::in, tiny_window);
    const auto boundary = static_cast<std::streamoff>(stream.window_size());
    stream.seekg(boundary);
    stream.get();

    stream.unget();
    stream.unget();

    require(stream.get() == text[static_cast<std::size_t>(boundary) - 1]);
}

void test_seekg_moves_relative_to_the_end() {
    const temp_file file("ifs");
    write_all(file, "abcdef");
    ifmstream stream(file.path());

    stream.seekg(-2, std::ios::end);

    require(stream.tellg() == std::streampos(4));
    require(stream.get() == 'e');
}

void test_seekg_before_the_beginning_sets_failbit() {
    const temp_file file("ifs");
    write_all(file, "abc");
    ifmstream stream(file.path());

    stream.seekg(-1, std::ios::cur);

    require(stream.fail());
}

void test_seekg_past_the_end_reads_nothing() {
    const temp_file file("ifs");
    write_all(file, "abc");
    ifmstream stream(file.path());

    constexpr std::streamoff offset = 100;

    stream.seekg(offset);

    require(stream.tellg() == std::streampos(offset));
    require(stream.get() == ifmstream::traits_type::eof());
}

void test_seekg_clears_eofbit() {
    const temp_file file("ifs");
    write_all(file, "abc");
    ifmstream stream(file.path());
    std::string word;
    stream >> word;
    require(stream.eof());

    stream.seekg(0);

    require(stream.good());
    require(stream.get() == 'a');
}

void test_ate_starts_at_the_end() {
    const temp_file file("ifs");
    write_all(file, "abcdef");
    ifmstream stream(file.path(), std::ios::ate);

    require(stream.tellg() == std::streampos(6));
}

void test_an_empty_file_reads_nothing() {
    const temp_file file("ifs");
    write_all(file, "");
    ifmstream stream(file.path());

    require(stream.is_open());
    require(stream.get() == ifmstream::traits_type::eof());
}

void test_a_read_only_file_can_be_opened() {
    const temp_file file("ifs");
    write_all(file, "abc");
    std::filesystem::permissions(
        file.path(), std::filesystem::perms::owner_read,
        std::filesystem::perm_options::replace);

    ifmstream stream(file.path());

    require(stream.is_open());
    require(stream.get() == 'a');
}

void test_a_missing_file_sets_failbit() {
    const temp_file file("ifs");

    const ifmstream stream(file.path());

    require_false(stream.is_open());
    require(stream.fail());
}

void test_move_transfers_the_file() {
    const temp_file file("ifs");
    write_all(file, "abc");
    ifmstream stream(file.path());
    stream.get();

    ifmstream moved(std::move(stream));

    require(moved.is_open());
    require(moved.get() == 'b');
    require_false(stream.is_open()); // NOLINT(bugprone-use-after-move)
}

void test_swap_exchanges_files() {
    const temp_file first("ifs");
    const temp_file second("ifs");
    write_all(first, "first");
    write_all(second, "second");
    ifmstream lhs(first.path());
    ifmstream rhs(second.path());

    swap(lhs, rhs);

    require(lhs.get() == 's');
    require(rhs.get() == 'f');
}

} // namespace

extern "C++" int main() {
    test_getline_crosses_windows();
    test_formatted_input_parses_numbers();
    test_get_and_peek();
    test_read_stops_at_the_end();
    test_rdbuf_copies_the_whole_file();
    test_unget_crosses_windows();
    test_seekg_moves_relative_to_the_end();
    test_seekg_before_the_beginning_sets_failbit();
    test_seekg_past_the_end_reads_nothing();
    test_seekg_clears_eofbit();
    test_ate_starts_at_the_end();
    test_an_empty_file_reads_nothing();
    test_a_read_only_file_can_be_opened();
    test_a_missing_file_sets_failbit();
    test_move_transfers_the_file();
    test_swap_exchanges_files();
    return neutron_test::result();
}
