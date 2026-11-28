// IWYU pragma: private, include <neutron/algorithm.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Lhs, typename Rhs>
constexpr auto min(Lhs&& lhs, Rhs&& rhs) -> std::common_type_t<Lhs, Rhs> {
    return lhs <= rhs ? std::forward<Lhs>(lhs) : std::forward<Rhs>(rhs);
}

ATOM_EXPORT template <typename Lhs, typename Rhs, typename... Others>
requires(sizeof...(Others) != 0)
constexpr auto min(Lhs&& lhs, Rhs&& rhs, Others&&... others)
    -> std::common_type_t<Lhs, Rhs, Others...> {
    return min(
        min(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)),
        std::forward<Others>(others)...);
}

ATOM_EXPORT template <typename Lhs, typename Rhs>
constexpr auto max(Lhs&& lhs, Rhs&& rhs) -> std::common_type_t<Lhs, Rhs> {
    return lhs >= rhs ? std::forward<Lhs>(lhs) : std::forward<Rhs>(rhs);
}

ATOM_EXPORT template <typename Lhs, typename Rhs, typename... Others>
requires(sizeof...(Others) != 0)
constexpr auto max(Lhs&& lhs, Rhs&& rhs, Others&&... others)
    -> std::common_type_t<Lhs, Rhs, Others...> {
    return max(
        max(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)),
        std::forward<Others>(others)...);
}

} // namespace neutron
