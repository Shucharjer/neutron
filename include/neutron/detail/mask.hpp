#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <bit>
    #include <concepts>
    #include <cstddef>
#endif

namespace neutron {

template <std::size_t Size>
concept _single_bit = std::has_single_bit(Size);

template <std::size_t Dev, std::unsigned_integral Uint = std::size_t>
constexpr Uint _uint_dev(Uint num) noexcept {
    if constexpr (std::has_single_bit(Dev)) {
        constexpr auto mask = std::countr_zero(Dev);
        return num >> mask;
    } else {
        return num / Dev;
    }
}

template <std::size_t Mod, std::unsigned_integral Uint = std::size_t>
constexpr Uint _uint_mod(Uint num) noexcept {
    if constexpr (std::has_single_bit(Mod)) {
        constexpr auto mask = Mod - 1;
        return num & mask;
    } else {
        return num % Mod;
    }
}

} // namespace neutron
