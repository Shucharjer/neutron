// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <array>
#include <cstddef>

namespace neutron {

template <typename T>
constexpr bool has_fixed_size = false;

template <typename T>
concept fixed_array = has_fixed_size<T>;

template <typename T>
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
