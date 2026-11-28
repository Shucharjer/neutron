// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <array>
    #include <cstddef>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename T>
constexpr bool has_fixed_size = false;

ATOM_EXPORT template <typename T>
concept fixed_array = has_fixed_size<T>;

ATOM_EXPORT template <typename T>
constexpr std::size_t fixed_size = 0;

template <typename T, std::size_t Size>
constexpr bool has_fixed_size<T[Size]> = true;
template <typename T, std::size_t Size>
constexpr bool has_fixed_size<std::array<T, Size>> = true;

template <typename T, std::size_t Size>
constexpr std::size_t fixed_size<T[Size]> = Size;
template <typename T, std::size_t Size>
constexpr std::size_t fixed_size<std::array<T, Size>> = Size;

} // namespace neutron
