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

int main() {

    // bitstream
    {

        // scalar

        {
            char val = 32;
            std::byte buf[1];
            bitstream bit{ buf, 1 };
            serialize(bit, val);
        }

// NOLINTNEXTLINE
#define TEST_SCALAR(type)                                                      \
    do {                                                                       \
        type val = 32;                                                         \
        std::array<std::byte, sizeof(type)> buf{};                             \
        bitstream bit{ buf.data(), sizeof(type) };                             \
        serialize(bit, val);                                                   \
        bit = bitstream{ buf.data(), sizeof(type) };                            \
        type another;                                                          \
        deserialize(bit, another);                                             \
        require_or_return(val == another, 1);                                  \
    } while (false)

        TEST_SCALAR(std::uint8_t);
        TEST_SCALAR(std::int8_t);
        TEST_SCALAR(std::uint16_t);
        TEST_SCALAR(std::int16_t);
        TEST_SCALAR(std::uint32_t);
        TEST_SCALAR(std::int32_t);
        TEST_SCALAR(std::uint64_t);
        TEST_SCALAR(std::int64_t);
        TEST_SCALAR(std::uint64_t);
        TEST_SCALAR(std::int64_t);
#if defined(__SIZEOF_INT128__)
        TEST_SCALAR(__uint128_t);
        TEST_SCALAR(__int128_t);
#endif
        TEST_SCALAR(float);
        TEST_SCALAR(double);

#undef TEST_SCALAR

        // fixed

        {
            constexpr auto size = 32;
            {
                int array[size];
                std::iota(array, array + size, 0);
                std::array<std::byte, sizeof(int) * size> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, array);
                int another[size]{};
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(std::ranges::equal(array, another), 1);
            }

            {
                std::array<int, size> array{};
                std::iota(array.begin(), array.end(), 0);
                std::array<std::byte, sizeof(int) * size> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, array);
                std::array<int, size> another{};
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(std::ranges::equal(array, another), 1);
            }

            {
                int array[size]{};
                std::iota(array, array + size, 0);
                std::array<std::byte, sizeof(int) * size> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, array);
                std::array<int, size> another{};
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(std::ranges::equal(array, another), 1);
            }
        }

        // tuple
        {
            {
                std::tuple<int, int> tuple;
                get<0>(tuple) = 32;
                get<1>(tuple) = 64;

                std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, tuple);

                std::tuple<int, int> another;
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(
                    get<0>(tuple) == get<0>(another) &&
                        get<1>(tuple) == get<1>(another),
                    1);
            }

            {
                shared_tuple<int, int> tuple;
                get<0>(tuple) = 32;
                get<1>(tuple) = 64;

                std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, tuple);

                shared_tuple<int, int> another;
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(
                    get<0>(tuple) == get<0>(another) &&
                        get<1>(tuple) == get<1>(another),
                    1);
            }

            {
                shared_tuple<int, int> tuple;
                get<0>(tuple) = 32;
                get<1>(tuple) = 64;

                std::array<std::byte, sizeof(std::tuple<int, int>)> buf{};
                bitstream bs{ buf.data(), buf.size() };
                serialize(bs, tuple);

                std::tuple<int, int> another;
                bs = bitstream{ buf.data(), buf.size() };
                deserialize(bs, another);

                require_or_return(
                    get<0>(tuple) == get<0>(another) &&
                        get<1>(tuple) == get<1>(another),
                    1);
            }
        }

        // contiguous range

        {
            constexpr auto cnt = 32;
            std::vector<int> vec(cnt);
            std::iota(vec.begin(), vec.end(), 0);
            std::array<std::byte, sizeof(std::size_t) + cnt * sizeof(int)> buf;
            bitstream bs{ buf.data(), buf.size() };
            serialize(bs, vec);

            std::vector<int> another;
            bs = bitstream{ buf.data(), buf.size() };
            deserialize(bs, another);

            require_or_return(std::ranges::equal(vec, another), 1);
        }

        {
            struct eagg {};
            bitstream bs;
            eagg agg{};
            serialize(bs, agg);

            deserialize(bs, agg);
        }
    }

    return 0;
}
