#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <neutron/fstream.hpp>
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

/// A window size of 1 is rounded up to the mapping granularity, so these
/// files span several windows.
constexpr std::size_t tiny_window = 1;

constexpr auto eof = fmbuf::traits_type::eof();

std::string read_all(const temp_file& file) {
    std::ifstream stream(file.path(), std::ios::binary);
    return { std::istreambuf_iterator<char>(stream),
             std::istreambuf_iterator<char>() };
}

void write_all(const temp_file& file, const std::string& text) {
    std::ofstream stream(file.path(), std::ios::binary);
    stream << text;
}

/// Text spanning two windows, so that its middle is a window boundary.
std::string two_windows(const fmbuf& buffer) {
    std::string text(buffer.window_size() * 2, 'a');
    text[buffer.window_size() - 1] = 'x';
    text[buffer.window_size()]     = 'y';
    return text;
}

void test_open_rejects_invalid_modes() {
    const temp_file file("fmb");
    write_all(file, "abc");
    fmbuf buffer;

    require(
        buffer.open(file.path(), std::ios::in | std::ios::trunc) == nullptr);
    require(
        buffer.open(file.path(), std::ios::app | std::ios::trunc) == nullptr);
    require(buffer.open(file.path(), std::ios::trunc) == nullptr);
    require(buffer.open(file.path(), std::ios::openmode{}) == nullptr);
    require_false(buffer.is_open());
}

void test_open_fails_when_already_open() {
    const temp_file file("fmb");
    write_all(file, "abc");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);

    require(buffer.open(file.path(), std::ios::in) == nullptr);
    require(buffer.is_open());
}

void test_open_rejects_a_zero_window_size() {
    const temp_file file("fmb");
    fmbuf buffer;

    require(buffer.open(file.path(), std::ios::out, 0) == nullptr);
}

void test_window_size_is_rounded_to_the_granularity() {
    const temp_file file("fmb");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::out, tiny_window);

    require(buffer.window_size() == internal::file_mapping::granularity());
}

void test_close_fails_without_a_file() {
    fmbuf buffer;

    require(buffer.close() == nullptr);
}

void test_close_truncates_to_the_written_size() {
    constexpr std::string_view digits = "0123456789";
    const temp_file file("fmb");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::out, tiny_window);
    buffer.sputn(digits.data(), digits.size());

    require(buffer.close() == &buffer);
    require(read_all(file) == "0123456789");
}

void test_sync_truncates_to_the_written_size() {
    constexpr std::string_view name = "neutron";
    const temp_file file("fmb");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::out, tiny_window);
    buffer.sputn(name.data(), name.size());

    require(buffer.pubsync() == 0);
    require(std::filesystem::file_size(file.path()) == name.size());

    buffer.sputc('!');
    buffer.close();

    require(read_all(file) == "neutron!");
}

void test_putback_crosses_windows() {
    const temp_file file("fmb");
    fmbuf buffer;
    buffer.open(
        file.path(), std::ios::in | std::ios::out | std::ios::trunc,
        tiny_window);
    const auto text = two_windows(buffer);
    buffer.sputn(text.data(), static_cast<std::streamsize>(text.size()));
    buffer.pubseekpos(static_cast<std::streamoff>(buffer.window_size()));

    require(buffer.sgetc() == 'y');
    require(buffer.sputbackc('x') == 'x');
    require(buffer.sgetc() == 'x');
}

void test_putback_of_a_different_character_fails() {
    const temp_file file("fmb");
    write_all(file, "ab");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in | std::ios::out);
    buffer.sbumpc();

    require(buffer.sputbackc('z') == eof);
    require(buffer.sgetc() == 'b');
    require(read_all(file) == "ab");
}

void test_putback_at_the_beginning_fails() {
    const temp_file file("fmb");
    write_all(file, "ab");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);

    require(buffer.sungetc() == eof);
}

void test_in_avail_counts_the_rest_of_the_file() {
    const temp_file file("fmb");
    write_all(file, "abcdef");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);

    require(buffer.in_avail() == 6);
    buffer.pubseekoff(0, std::ios::end);
    require(buffer.in_avail() == -1);
}

void test_an_output_buffer_cannot_be_read() {
    const temp_file file("fmb");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::out);
    buffer.sputn("abc", 3);
    buffer.pubseekpos(0);

    require(buffer.sgetc() == eof);
}

void test_an_input_buffer_cannot_be_written() {
    const temp_file file("fmb");
    write_all(file, "abc");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);

    require(buffer.sputc('z') == eof);
    require(buffer.sgetc() == 'a');
}

void test_seeking_before_the_beginning_fails() {
    const temp_file file("fmb");
    write_all(file, "abc");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);
    buffer.pubseekpos(1);

    require(buffer.pubseekoff(-2, std::ios::cur) == std::streampos(-1));
    require(buffer.pubseekoff(0, std::ios::cur) == std::streampos(1));
}

void test_seeking_a_closed_buffer_fails() {
    fmbuf buffer;

    require(buffer.pubseekoff(0, std::ios::cur) == std::streampos(-1));
}

#ifdef __cpp_lib_ios_noreplace
void test_noreplace_requires_a_new_file() {
    const temp_file file("fmb");
    write_all(file, "kept");
    fmbuf buffer;

    require(
        buffer.open(file.path(), std::ios::out | std::ios::noreplace) ==
        nullptr);
    require(read_all(file) == "kept");
}
#endif

void test_move_keeps_the_area() {
    const temp_file file("fmb");
    write_all(file, "abc");
    fmbuf buffer;
    buffer.open(file.path(), std::ios::in);
    buffer.sbumpc();

    fmbuf moved(std::move(buffer));

    require(moved.sgetc() == 'b');
    require_false(buffer.is_open()); // NOLINT(bugprone-use-after-move)
}

void test_swap_exchanges_files() {
    const temp_file first("fmb");
    const temp_file second("fmb");
    write_all(first, "first");
    write_all(second, "second");
    fmbuf lhs;
    fmbuf rhs;
    lhs.open(first.path(), std::ios::in);
    rhs.open(second.path(), std::ios::in);
    lhs.sbumpc();

    swap(lhs, rhs);

    require(lhs.sgetc() == 's');
    require(rhs.sgetc() == 'i');
}

void test_wide_characters_round_trip() {
    const temp_file file("fmb");
    const std::wstring text = L"neutron";
    {
        wfmbuf buffer;
        buffer.open(file.path(), std::ios::out);
        buffer.sputn(text.data(), static_cast<std::streamsize>(text.size()));
    }
    wfmbuf buffer;
    buffer.open(file.path(), std::ios::in);
    std::wstring copy(text.size(), L'\0');

    require(
        buffer.sgetn(copy.data(), static_cast<std::streamsize>(copy.size())) ==
        static_cast<std::streamsize>(text.size()));
    require(copy == text);
    require(
        std::filesystem::file_size(file.path()) ==
        text.size() * sizeof(wchar_t));
}

} // namespace

int main() {
    test_open_rejects_invalid_modes();
    test_open_fails_when_already_open();
    test_open_rejects_a_zero_window_size();
    test_window_size_is_rounded_to_the_granularity();
    test_close_fails_without_a_file();
    test_close_truncates_to_the_written_size();
    test_sync_truncates_to_the_written_size();
    test_putback_crosses_windows();
    test_putback_of_a_different_character_fails();
    test_putback_at_the_beginning_fails();
    test_in_avail_counts_the_rest_of_the_file();
    test_an_output_buffer_cannot_be_read();
    test_an_input_buffer_cannot_be_written();
    test_seeking_before_the_beginning_fails();
    test_seeking_a_closed_buffer_fails();
#ifdef __cpp_lib_ios_noreplace
    test_noreplace_requires_a_new_file();
#endif
    test_move_keeps_the_area();
    test_swap_exchanges_files();
    test_wide_characters_round_trip();
    return neutron_test::result();
}
