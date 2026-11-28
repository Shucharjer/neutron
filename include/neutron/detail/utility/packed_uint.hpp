#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <cstdint>
    #include <limits>
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <std::size_t Size>
using packed_size_t = std::conditional_t<
    (Size < (std::numeric_limits<std::uint8_t>::max)()), std::uint8_t,
    std::conditional_t<
        (Size < (std::numeric_limits<std::uint16_t>::max)()), std::uint16_t,
        std::conditional_t<
            (Size < (std::numeric_limits<std::uint32_t>::max)()), std::uint32_t,
            std::uint64_t>>>;

ATOM_EXPORT template <std::size_t Size>
using packed_uint_t = std::conditional_t<
    (Size <= (std::numeric_limits<std::uint8_t>::max)()), std::uint8_t,
    std::conditional_t<
        (Size <= (std::numeric_limits<std::uint16_t>::max)()), std::uint16_t,
        std::conditional_t<
            (Size <= (std::numeric_limits<std::uint32_t>::max)()),
            std::uint32_t, std::uint64_t>>>;

ATOM_EXPORT template <std::size_t Size>
using fast_packed_size_t = std::conditional_t<
    (Size < (std::numeric_limits<std::uint8_t>::max)()), std::uint_fast8_t,
    std::conditional_t<
        (Size < (std::numeric_limits<std::uint16_t>::max)()),
        std::uint_fast16_t,
        std::conditional_t<
            (Size < (std::numeric_limits<std::uint32_t>::max)()),
            std::uint_fast32_t, std::uint_fast64_t>>>;

ATOM_EXPORT template <std::size_t Size>
using fast_packed_uint_t = std::conditional_t<
    (Size <= (std::numeric_limits<std::uint8_t>::max)()), std::uint_fast8_t,
    std::conditional_t<
        (Size <= (std::numeric_limits<std::uint16_t>::max)()),
        std::uint_fast16_t,
        std::conditional_t<
            (Size <= (std::numeric_limits<std::uint32_t>::max)()),
            std::uint_fast32_t, std::uint_fast64_t>>>;

} // namespace neutron
