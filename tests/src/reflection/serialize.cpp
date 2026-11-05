#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <ranges>
#include <tuple>
#include <vector>
#include <neutron/reflection.hpp>
#include <neutron/tuple.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

constexpr auto element_count = 32;

template <typename T>
void require_scalar_round_trip() {
    T val = 32;
    std::array<std::byte, sizeof(T)> buf{};
    bitstream bit{ buf.data(), sizeof(T) };

    serialize(bit, val);
    bit = bitstream{ buf.data(), sizeof(T) };
    T another;
    deserialize(bit, another);

    require(val == another);
}

void test_char_serializes_into_one_byte() {
    char val = 32;
    std::byte buf[1];
    bitstream bit{ buf, 1 };

    serialize(bit, val);
}

void test_scalars_round_trip() {
    require_scalar_round_trip<std::uint8_t>();
    require_scalar_round_trip<std::int8_t>();
    require_scalar_round_trip<std::uint16_t>();
    require_scalar_round_trip<std::int16_t>();
    require_scalar_round_trip<std::uint32_t>();
    require_scalar_round_trip<std::int32_t>();
    require_scalar_round_trip<std::uint64_t>();
    require_scalar_round_trip<std::int64_t>();
    require_scalar_round_trip<std::uint64_t>();
    require_scalar_round_trip<std::int64_t>();
#if defined(__SIZEOF_INT128__)
    require_scalar_round_trip<__uint128_t>();
    require_scalar_round_trip<__int128_t>();
#endif
    require_scalar_round_trip<float>();
    require_scalar_round_trip<double>();
}

void test_c_array_round_trips() {
    int array[element_count];
    std::iota(array, array + element_count, 0);
    std::array<std::byte, sizeof(int) * element_count> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, array);
    int another[element_count]{};
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(std::ranges::equal(array, another));
}

void test_std_array_round_trips() {
    std::array<int, element_count> array{};
    std::iota(array.begin(), array.end(), 0);
    std::array<std::byte, sizeof(int) * element_count> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, array);
    std::array<int, element_count> another{};
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(std::ranges::equal(array, another));
}

void test_c_array_deserializes_into_std_array() {
    int array[element_count]{};
    std::iota(array, array + element_count, 0);
    std::array<std::byte, sizeof(int) * element_count> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, array);
    std::array<int, element_count> another{};
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(std::ranges::equal(array, another));
}

void test_std_tuple_round_trips() {
    std::tuple<int, int> tuple;
    get<0>(tuple) = 32;
    get<1>(tuple) = 64;
    std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, tuple);
    std::tuple<int, int> another;
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(get<0>(tuple) == get<0>(another));
    require(get<1>(tuple) == get<1>(another));
}

void test_shared_tuple_round_trips() {
    shared_tuple<int, int> tuple;
    get<0>(tuple) = 32;
    get<1>(tuple) = 64;
    std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, tuple);
    shared_tuple<int, int> another;
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(get<0>(tuple) == get<0>(another));
    require(get<1>(tuple) == get<1>(another));
}

void test_shared_tuple_deserializes_into_std_tuple() {
    shared_tuple<int, int> tuple;
    get<0>(tuple) = 32;
    get<1>(tuple) = 64;
    std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, tuple);
    std::tuple<int, int> another;
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(get<0>(tuple) == get<0>(another));
    require(get<1>(tuple) == get<1>(another));
}

void test_vector_round_trips_with_size_prefix() {
    std::vector<int> vec(element_count);
    std::iota(vec.begin(), vec.end(), 0);
    std::array<std::byte, sizeof(std::size_t) + element_count * sizeof(int)>
        buf;
    bitstream bs{ buf.data(), buf.size() };

    serialize(bs, vec);
    std::vector<int> another;
    bs = bitstream{ buf.data(), buf.size() };
    deserialize(bs, another);

    require(std::ranges::equal(vec, another));
}

void test_empty_aggregate_needs_no_storage() {
    struct eagg {};
    bitstream bs;
    eagg agg{};

    serialize(bs, agg);
    deserialize(bs, agg);
}

} // namespace

int main() {
    test_char_serializes_into_one_byte();
    test_scalars_round_trip();
    test_c_array_round_trips();
    test_std_array_round_trips();
    test_c_array_deserializes_into_std_array();
    test_std_tuple_round_trips();
    test_shared_tuple_round_trips();
    test_shared_tuple_deserializes_into_std_tuple();
    test_vector_round_trips_with_size_prefix();
    test_empty_aggregate_needs_no_storage();
    return neutron_test::result();
}
