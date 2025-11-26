#pragma once
#include <concepts>
#include <tuple>
#include "./concepts.hpp"

namespace neutron {

namespace _reflection {

/**
 * @brief Universal type for deducing.
 *
 * If you define the operator, you may make things confusing!
 * like this: std::vector<int> vec(universal{});
 */
struct universal {
    template <typename Ty>
    operator Ty();
};

template <typename Ty, typename... Args>
constexpr auto member_count_of_impl() noexcept {
    if constexpr (std::constructible_from<Ty, Args..., universal>) {
        return member_count_of_impl<Ty, Args..., universal>();
    } else {
        return sizeof...(Args);
    }
}

/**
 * @brief Get the member count of a type.
 *
 */
template <reflectible Ty>
consteval auto member_count_of() noexcept {
    using pure_t = std::remove_cvref_t<Ty>;
    if constexpr (default_reflectible_aggregate<Ty>) {
        return member_count_of_impl<pure_t>();
    } else { // has_field_traits
        return std::tuple_size_v<decltype(pure_t::field_traits())>;
    }
}

} // namespace _reflection

using _reflection::member_count_of;

} // namespace neutron
