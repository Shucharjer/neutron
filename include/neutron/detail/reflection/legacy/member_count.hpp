// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <tuple>
#endif
#include "neutron/detail/reflection/legacy/concepts.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

namespace _refl_legacy {

/**
 * @brief Universal type for deducing.
 *
 * If you define the operator, you may make things confusing!
 * like this: std::vector<int> vec(universal<std::vector<int>>{});
 */
template <typename T>
struct universal {
    template <typename Ty>
    requires(!std::same_as<std::remove_cvref_t<Ty>, T>)
    operator Ty();
};

template <typename Ty, typename... Args>
constexpr auto member_count_of_impl() noexcept {
    if constexpr (std::constructible_from<Ty, Args..., universal<Ty>>) {
        return member_count_of_impl<Ty, Args..., universal<Ty>>();
    } else {
        return sizeof...(Args);
    }
}

/**
 * @brief Get the member count of a type.
 *
 */
ATOM_EXPORT template <reflectible Ty>
consteval auto member_count_of() noexcept {
    using pure_t = std::remove_cvref_t<Ty>;
    if constexpr (default_reflectible_aggregate<Ty>) {
        return member_count_of_impl<pure_t>();
    } else { // has_field_traits
        return std::tuple_size_v<decltype(pure_t::field_traits())>;
    }
}

} // namespace _refl_legacy

ATOM_EXPORT using _refl_legacy::member_count_of;

} // namespace neutron
