// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <neutron/concepts.hpp>
#include "neutron/detail/concepts/trivially_relocatable.hpp"
#include "neutron/detail/concepts/tuple_like.hpp"

namespace neutron {

struct type_traits {
    std::uint64_t size : 32;
    std::uint64_t align : 16;
    std::uint64_t is_trivially_default_constructible : 1;
    std::uint64_t is_copy_constructible : 1;
    std::uint64_t is_trivially_copyable : 1;
    std::uint64_t is_trivially_copy_assignable : 1;
    std::uint64_t is_trivially_move_assignable : 1;
    std::uint64_t is_trivially_relocatible : 1;
    std::uint64_t is_trivially_destructible : 1;
    std::uint64_t is_scalar : 1;
    std::uint64_t is_object : 1;
    std::uint64_t is_bounded_array : 1;
    std::uint64_t is_enum : 1;
    std::uint64_t is_range : 1;
    std::uint64_t is_map_like : 1;
    std::uint64_t is_tuple_like : 1;
};

static_assert(
    sizeof(type_traits) == sizeof(std::uint64_t), "it should be easy to copy");

template <typename T>
consteval auto type_traits_of() noexcept -> type_traits {
    static_assert(sizeof(T) <= std::uint64_t(static_cast<std::uint32_t>(-1)));
    static_assert(alignof(T) <= std::uint64_t(static_cast<std::uint16_t>(-1)));

    return type_traits{
        .size  = sizeof(T),
        .align = alignof(T),
        .is_trivially_default_constructible =
            std::is_trivially_default_constructible_v<T>,
        .is_copy_constructible        = std::is_copy_constructible_v<T>,
        .is_trivially_copyable        = std::is_trivially_copyable_v<T>,
        .is_trivially_copy_assignable = std::is_trivially_copy_assignable_v<T>,
        .is_trivially_move_assignable = std::is_trivially_move_assignable_v<T>,
        .is_trivially_relocatible     = trivially_relocatable<T>,
        .is_trivially_destructible    = std::is_trivially_destructible_v<T>,
        .is_scalar                    = std::is_scalar_v<T>,
        .is_object                    = std::is_object_v<T>,
        .is_bounded_array             = std::is_bounded_array_v<T>,
        .is_enum                      = std::is_enum_v<T>,
        .is_range                     = std::ranges::range<T>,
        .is_map_like                  = map_like<T>,
        .is_tuple_like                = tuple_like<T>
    }; // namespace neutron
}

} // namespace neutron
