#pragma once
#include <bit>
#include <concepts>

namespace neutron {

template <size_t Size>
concept _single_bit = std::has_single_bit(Size);

template <std::unsigned_integral Uint>
constexpr Uint _uint_get_pow2_n(Uint num) noexcept {
    return std::bit_width(num) - 1;
}

template <size_t Dev, std::unsigned_integral Uint = size_t>
constexpr Uint _uint_dev(Uint num) noexcept {
    if constexpr (std::has_single_bit(Dev)) {
        constexpr auto mask = _uint_get_pow2_n(Dev);
        return num >> mask;
    } else {
        return num / Dev;
    }
}

template <size_t Mod, std::unsigned_integral Uint = size_t>
constexpr Uint _uint_mod(Uint num) noexcept {
    if constexpr (std::has_single_bit(Mod)) {
        constexpr auto mask = Mod - 1;
        return num & mask;
    } else {
        return num % Mod;
    }
}

} // namespace neutron
