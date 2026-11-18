#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <neutron/fstream.hpp>
#include "neutron/detail/utility/macros.hpp"
#include "require.hpp"
#include "temp_file.hpp"

using namespace neutron;
using neutron_test::temp_file;

namespace {

/// Content used by the round trip; the terminating null is written too.
constexpr char text[] = "neutron"; // NOLINT(modernize-avoid-c-arrays)

constexpr std::size_t grown_size = 4096;

template <typename Exception, typename Fn>
bool throws(Fn&& fn) {
    ATOM_TRY { std::forward<Fn>(fn)(); }
    ATOM_CATCH(const Exception&) { return true; }
    return false;
}

void test_written_bytes_survive_reopening() {
    const temp_file file("map");
    {
        file_map<false> out(file.string(), file_creation::truncate);
        out.resize(sizeof(text));
        require(out.size() == sizeof(text));
        require_or_return(out.data() != nullptr, );

        std::memcpy(out.data(), text, sizeof(text));
        out.flush();
    }

    const file_map<true> in(file.string());

    require(in.size() == sizeof(text));
    require(std::memcmp(in.data(), text, sizeof(text)) == 0);
}

void test_grown_part_reads_as_zeros() {
    const temp_file file("map");
    file_map<false> map(file.string(), file_creation::truncate);
    map.resize(1);
    map[0] = std::byte{ 0xAB };

    map.resize(grown_size);

    require(map.size() == grown_size);
    require(map[0] == std::byte{ 0xAB });
    require(map[grown_size - 1] == std::byte{ 0 });
}

void test_at_throws_out_of_range() {
    const temp_file file("map");
    const auto path = file.string();
    { const file_map<false> created(path, file_creation::truncate); }

    const file_map<true> map(path);

    require(map.empty());
    require(throws<std::out_of_range>([&] {
        (void)map.at(0);
    }));
}

void test_opening_missing_file_throws() {
    const temp_file file("map");

    require(throws<std::system_error>([&] {
        const file_map<true> map(file.string());
    }));
}

void test_move_transfers_the_mapping() {
    const temp_file file("map");
    file_map<false> map(file.string(), file_creation::truncate);
    map.resize(grown_size);
    const auto* const data = map.data();
    require_or_return(data != nullptr, );

    const file_map<false> moved(std::move(map));

    require(moved.data() == data);
    require(moved.size() == grown_size);
    require(map.data() == nullptr); // NOLINT(bugprone-use-after-move)
}

void test_read_only_map_exposes_const_bytes() {
    // A read-only map yields const references only.
    static_assert(std::is_const_v<file_map<true>::element_type>);
    static_assert(std::is_same_v<file_map<true>::iterator, const std::byte*>);
    static_assert(!std::is_const_v<file_map<false>::element_type>);
}

} // namespace

int main() {
    test_written_bytes_survive_reopening();
    test_grown_part_reads_as_zeros();
    test_at_throws_out_of_range();
    test_opening_missing_file_throws();
    test_move_transfers_the_mapping();
    test_read_only_map_exposes_const_bytes();
    return neutron_test::result();
}
