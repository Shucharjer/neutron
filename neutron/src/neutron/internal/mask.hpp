#pragma once
#include <bit>
#include <concepts>
#include <cstddef>

namespace neutron {

template <size_t Size>
concept _single_bit = std::has_single_bit(Size);

template <size_t Dev, std::unsigned_integral Uint = size_t>
constexpr Uint _uint_dev(Uint num) noexcept {
    if constexpr (std::has_single_bit(Dev)) {
        constexpr auto mask = std::countr_zero(Dev);
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
