#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace neutron {

template <size_t Size>
using packed_size_t = std::conditional_t<
    (Size < (std::numeric_limits<uint8_t>::max)()), uint8_t,
    std::conditional_t<
        (Size < (std::numeric_limits<uint16_t>::max)()), uint16_t,
        std::conditional_t<
            (Size < (std::numeric_limits<uint32_t>::max)()), uint32_t,
            uint64_t>>>;

template <size_t Size>
using packed_uint_t = std::conditional_t<
    (Size <= (std::numeric_limits<uint8_t>::max)()), uint8_t,
    std::conditional_t<
        (Size <= (std::numeric_limits<uint16_t>::max)()), uint16_t,
        std::conditional_t<
            (Size <= (std::numeric_limits<uint32_t>::max)()), uint32_t,
            uint64_t>>>;

template <size_t Size>
using fast_packed_size_t = std::conditional_t<
    (Size < (std::numeric_limits<uint8_t>::max)()), uint_fast8_t,
    std::conditional_t<
        (Size < (std::numeric_limits<uint16_t>::max)()), uint_fast16_t,
        std::conditional_t<
            (Size < (std::numeric_limits<uint32_t>::max)()), uint_fast32_t,
            uint_fast64_t>>>;

template <size_t Size>
using fast_packed_uint_t = std::conditional_t<
    (Size <= (std::numeric_limits<uint8_t>::max)()), uint_fast8_t,
    std::conditional_t<
        (Size <= (std::numeric_limits<uint16_t>::max)()), uint_fast16_t,
        std::conditional_t<
            (Size <= (std::numeric_limits<uint32_t>::max)()), uint_fast32_t,
            uint_fast64_t>>>;

} // namespace neutron
