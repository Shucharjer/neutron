#pragma once
#include <type_traits>

namespace neutron {

template <typename Lhs, typename Rhs>
constexpr auto min(Lhs&& lhs, Rhs&& rhs) -> std::common_type_t<Lhs, Rhs> {
    return lhs <= rhs ? std::forward<Lhs>(lhs) : std::forward<Rhs>(rhs);
}

template <typename Lhs, typename Rhs, typename... Others>
requires(sizeof...(Others) != 0)
constexpr auto min(Lhs&& lhs, Rhs&& rhs, Others&&... others)
    -> std::common_type_t<Lhs, Rhs, Others...> {
    return min(
        min(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)),
        std::forward<Others>(others)...);
}

template <typename Lhs, typename Rhs>
constexpr auto max(Lhs&& lhs, Rhs&& rhs) -> std::common_type_t<Lhs, Rhs> {
    return lhs >= rhs ? std::forward<Lhs>(lhs) : std::forward<Rhs>(rhs);
}

template <typename Lhs, typename Rhs, typename... Others>
requires(sizeof...(Others) != 0)
constexpr auto max(Lhs&& lhs, Rhs&& rhs, Others&&... others)
    -> std::common_type_t<Lhs, Rhs, Others...> {
    return max(
        max(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)),
        std::forward<Others>(others)...);
}

} // namespace neutron
