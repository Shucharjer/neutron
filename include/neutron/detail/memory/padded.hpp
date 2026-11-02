#pragma once
#include <cstddef>
#include <type_traits>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/memory/align.hpp"

namespace neutron {

template <typename T>
struct padded;

template <typename T>
requires std::is_empty_v<T>
struct padded<T> : T {
private:
    std::byte _[hdi_size]; // NOLINT
};

template <typename T>
requires(!std::is_empty_v<T>)
struct padded<T> : T {
private:
    std::byte _[align_up(sizeof(T), hdi_size) - sizeof(T)]; // NOLINT
};

} // namespace neutron
