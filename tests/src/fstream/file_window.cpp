#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <neutron/fstream.hpp>
#include "neutron/detail/utility/macros.hpp"
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

template <typename Exception, typename Fn>
bool throws(Fn&& fn) {
    ATOM_TRY { std::forward<Fn>(fn)(); }
    ATOM_CATCH(const Exception&) { return true; }
    return false;
}

/// Opens `path` for writing with a one-page window and returns that size.
std::size_t make_growable(const std::string& path) {
    file_window<false> window(path, 1, file_creation::truncate);
    window.resize(0);
    const std::size_t step = window.window_size();
    window.resize(step * 3 + step / 2);
    return step;
}

void test_windows_tile_the_file() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);

    const file_window<true> window(path, step);

    require(window.window_size() == step);
    require(window.file_size() == step * 3 + step / 2);
    require(window.size() == 4);
    require(window[0].size() == step);
    require(window[3].size() == step / 2);
}

void test_access_maps_one_window_at_a_time() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    file_window<false> window(path, step);

    require(window.mapped_index() == file_window<false>::npos);
    auto bytes = window[1];

    require(window.mapped_index() == 1);
    require(bytes.size() == step);
    require(bytes.data() != nullptr);
}

void test_accessing_the_same_window_keeps_the_mapping() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    file_window<false> window(path, step);

    const auto* const first  = window[2].data();
    const auto* const second = window[2].data();

    require(first == second);
}

void test_writes_reach_the_file() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    {
        file_window<false> window(path, step);
        window[0][0]        = std::byte{ 0x11 };
        window[1][step - 1] = std::byte{ 0x22 };
        window.flush();
    }

    const file_window<true> window(path, step);

    require(window[0][0] == std::byte{ 0x11 });
    require(window[1][step - 1] == std::byte{ 0x22 });
}

void test_window_at_finds_the_window_of_a_byte() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    const file_window<true> window(path, step);

    require(window.window_at(0).size() == step);
    require(window.window_at(step * 3).size() == step / 2);
}

void test_iteration_visits_every_window() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    const file_window<true> window(path, step);

    auto it   = window.begin();
    auto last = window.end();

    require(std::distance(it, last) == 4);
    require((*it).size() == step);
    require((*(last - 1)).size() == step / 2);
}

void test_out_of_range_is_reported() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    file_window<false> window(path, step);

    require(throws<std::out_of_range>([&] {
        (void)window.at(window.size());
    }));
}

void test_zero_window_size_throws_invalid_argument() {
    const temp_file file("wnd");

    require(throws<std::invalid_argument>([&] {
        const file_window<false> window(
            file.string(), 0, file_creation::truncate);
    }));
}

void test_move_transfers_the_file() {
    const temp_file file("wnd");
    const auto path        = file.string();
    const std::size_t step = make_growable(path);
    file_window<false> window(path, step);
    (void)window[1];

    const file_window<false> moved(std::move(window));

    require(moved.window_size() == step);
    require(moved.mapped_index() == 1);
    require(window.window_size() == 0); // NOLINT(bugprone-use-after-move)
}

} // namespace

int main() {
    test_windows_tile_the_file();
    test_access_maps_one_window_at_a_time();
    test_accessing_the_same_window_keeps_the_mapping();
    test_writes_reach_the_file();
    test_window_at_finds_the_window_of_a_byte();
    test_iteration_visits_every_window();
    test_out_of_range_is_reported();
    test_zero_window_size_throws_invalid_argument();
    test_move_transfers_the_file();
    return neutron_test::result();
}
