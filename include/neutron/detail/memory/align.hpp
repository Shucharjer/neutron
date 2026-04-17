#pragma once
#include <concepts>
#include <cstddef>

namespace neutron {

template <std::unsigned_integral T>
constexpr T align_up(T val, size_t align) noexcept {
    const auto mask = static_cast<T>(align - 1U);
    return (val + mask) & ~mask;
}

template <std::unsigned_integral T>
constexpr T align_down(T val, size_t align) noexcept {
    const auto mask = static_cast<T>(align - 1U);
    return val & ~mask;
}

} // namespace neutron
