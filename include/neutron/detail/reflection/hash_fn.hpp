#pragma once
#include <cstdint>
#include <string_view>
#include "neutron/detail/macros.hpp"
#include "neutron/detail/reflection/hash_t.hpp"

/*! @cond TURN_OFF_DOXYGEN */
namespace neutron::internal {

constexpr hash_t hash(std::string_view string) noexcept {
    // fnv1a
    constexpr uint32_t magic = 0x811c9dc5;
    constexpr uint32_t prime = 0x01000193;

    uint32_t value = magic;
    for (char ch : string) {
        value = value ^ static_cast<uint8_t>(ch);
        value *= prime;
    }

    return value;
}

template <std::ranges::range Rng>
requires std::same_as<std::ranges::range_value_t<Rng>, uint32_t>
ATOM_FORCE_INLINE constexpr long_hash_t
    hash_combine(const Rng& range) noexcept {
    constexpr long_hash_t magic  = 0xcbf29ce484222325;
    constexpr long_hash_t prime  = 0x100000001b3;
    constexpr uint8_t byte_all_1 = 0xffU;

    long_hash_t hash = magic;
    for (long_hash_t elem : range) {
        // for little-endian
        // NOLINTBEGIN: unroll
        hash ^= static_cast<std::uint8_t>(elem);
        hash *= prime;
        hash ^= static_cast<std::uint8_t>(elem >> 8U);
        hash *= prime;
        hash ^= static_cast<std::uint8_t>(elem >> 16U);
        hash *= prime;
        hash ^= static_cast<std::uint8_t>(elem >> 24U);
        hash *= prime;
        // NOLINTEND
    }

    return hash;
}

} // namespace neutron::internal
/*! @endcond */
