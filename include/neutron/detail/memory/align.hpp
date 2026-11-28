// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <cstddef>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <std::unsigned_integral T>
constexpr T align_up(T val, std::size_t align) noexcept {
    const auto mask = static_cast<T>(align - 1U);
    return (val + mask) & ~mask;
}

ATOM_EXPORT template <std::unsigned_integral T>
constexpr T align_down(T val, std::size_t align) noexcept {
    const auto mask = static_cast<T>(align - 1U);
    return val & ~mask;
}

} // namespace neutron
