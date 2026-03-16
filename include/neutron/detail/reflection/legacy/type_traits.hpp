// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <cstdint>
#include "neutron/detail/concepts/value.hpp"

namespace neutron {

using traits_base = uint64_t;

// clang-format off
enum traits_bits : traits_base {
    is_integral                        = 0x1U,
    is_floating_point                  = 0x1U << 1,
    is_enum                            = 0x1U << 2,
    is_union                           = 0x1U << 3,
    is_class                           = 0x1U << 4,
    is_object                          = 0x1U << 5,
    is_trivial                         = 0x1U << 6, // deprecated
    is_standard_layout                 = 0x1U << 7,
    is_empty                           = 0x1U << 8,
    is_polymorphic                     = 0x1U << 9,
    is_abstract                        = 0x1U << 10,
    is_final                           = 0x1U << 11,
    is_aggregate                       = 0x1U << 12,
    is_pointer                         = 0x1U << 13,
    is_default_constructible           = 0x1U << 14,
    is_trivially_default_constructible = 0x1U << 15,
    is_nothrow_default_constructible   = 0x1U << 16,
    is_copy_constructible              = 0x1U << 17,
    is_nothrow_copy_constructible      = 0x1U << 18,
    is_trivially_copy_constructible    = 0x1U << 19,
    is_move_constructible              = 0x1U << 20,
    is_trivially_move_constructible    = 0x1U << 21,
    is_nothrow_move_construcitble      = 0x1U << 22,
    is_copy_assignable                 = 0x1U << 23,
    is_trivially_copy_assignable       = 0x1U << 24,
    is_nothrow_copy_assignable         = 0x1U << 25,
    is_move_assignable                 = 0x1U << 26,
    is_trivially_move_assignable       = 0x1U << 27,
    is_nothrow_move_assignable         = 0x1U << 28,
    is_destructible                    = 0x1U << 29,
    is_trivially_destructible          = 0x1U << 30,
    is_nothrow_destructible            = 0x1U << 31,
    // reserve
    _reserve                           = 0xFFFFFFFFFFFFFFFF
};
// clang-format on

namespace _reflection {

template <value Ty>
consteval uint32_t _traits_of_type() noexcept {
    using enum traits_bits;
    traits_base mask = 0;
    mask |= std::is_integral_v<Ty> ? is_integral : 0;
    mask |= std::is_floating_point_v<Ty> ? is_floating_point : 0;
    mask |= std::is_enum_v<Ty> ? is_enum : 0;
    mask |= std::is_union_v<Ty> ? is_union : 0;
    mask |= std::is_class_v<Ty> ? is_class : 0;
    mask |= std::is_object_v<Ty> ? is_object : 0;
    mask |= std::is_pointer_v<Ty> ? is_pointer : 0;
    return mask;
}

template <value Ty>
consteval uint32_t _traits_of_ability() noexcept {
    using enum traits_bits;
    traits_base mask = 0;
    mask |= std::is_default_constructible_v<Ty> ? is_default_constructible : 0;
    mask |= std::is_copy_constructible_v<Ty> ? is_copy_constructible : 0;
    mask |= std::is_move_constructible_v<Ty> ? is_move_constructible : 0;
    mask |= std::is_copy_assignable_v<Ty> ? is_copy_assignable : 0;
    mask |= std::is_move_assignable_v<Ty> ? is_move_assignable : 0;
    mask |= std::is_destructible_v<Ty> ? is_destructible : 0;
    return mask;
}

template <value Ty>
consteval uint32_t _traits_of_nothrow() noexcept {
    using enum traits_bits;
    traits_base mask = 0;
    mask |= std::is_nothrow_default_constructible_v<Ty>
                ? is_nothrow_default_constructible
                : 0;
    mask |= std::is_nothrow_copy_constructible_v<Ty>
                ? is_nothrow_copy_constructible
                : 0;
    mask |= std::is_nothrow_move_constructible_v<Ty>
                ? is_nothrow_move_construcitble
                : 0;
    mask |=
        std::is_nothrow_copy_assignable_v<Ty> ? is_nothrow_copy_assignable : 0;
    mask |=
        std::is_nothrow_move_assignable_v<Ty> ? is_nothrow_move_assignable : 0;
    mask |= std::is_nothrow_destructible_v<Ty> ? is_nothrow_destructible : 0;
    return mask;
}

template <value Ty>
consteval uint32_t _traits_of_trivial() noexcept {
    using enum traits_bits;
    traits_base mask = 0;
    mask |= std::is_trivially_default_constructible_v<Ty>
                ? is_trivially_default_constructible
                : 0;
    mask |= std::is_trivially_copy_constructible_v<Ty>
                ? is_trivially_default_constructible
                : 0;
    mask |= std::is_trivially_move_constructible_v<Ty>
                ? is_trivially_move_constructible
                : 0;
    mask |= std::is_trivially_copy_assignable_v<Ty>
                ? is_trivially_copy_assignable
                : 0;
    mask |= std::is_trivially_move_assignable_v<Ty>
                ? is_trivially_move_assignable
                : 0;
    mask |=
        std::is_trivially_destructible_v<Ty> ? is_trivially_destructible : 0;
    return mask;
}

template <value Ty>
consteval auto traits_of() noexcept -> traits_bits {
    using enum traits_bits;
    traits_base mask = 0;
    mask |= std::is_standard_layout_v<Ty> ? is_standard_layout : 0;
    mask |= std::is_empty_v<Ty> ? is_empty : 0;
    mask |= std::is_polymorphic_v<Ty> ? is_polymorphic : 0;
    mask |= std::is_abstract_v<Ty> ? is_abstract : 0;
    mask |= std::is_final_v<Ty> ? is_final : 0;
    mask |= std::is_aggregate_v<Ty> ? is_aggregate : 0;
    mask |= _traits_of_type<Ty>() | _traits_of_ability<Ty>() |
            _traits_of_nothrow<Ty>() | _traits_of_trivial<Ty>();
    return static_cast<traits_bits>(mask);
}

template <value Ty>
constexpr bool authenticity_of(const traits_bits bits) noexcept {
    constexpr auto description = static_cast<traits_base>(description_of<Ty>());
    const auto mask            = static_cast<traits_base>(bits);
    const auto result          = description & mask;
    return result == mask;
}

struct authenticity_params {
    traits_bits desc;
    traits_bits bits;
};

constexpr bool authenticity_of(const authenticity_params params) noexcept {
    auto mask   = static_cast<traits_base>(params.bits);
    auto result = static_cast<traits_base>(params.desc) & mask;
    return result == mask;
}

} // namespace _reflection

using _reflection::traits_of;
using _reflection::authenticity_params;
using _reflection::authenticity_of;

} // namespace neutron
