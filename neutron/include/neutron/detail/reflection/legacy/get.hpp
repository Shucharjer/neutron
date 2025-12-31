#pragma once
#include <cstddef>
#include "neutron/tstring.hpp"
#include "./concepts.hpp"
#include "./tuple_view.hpp"

namespace neutron {

namespace _reflection {
/**
 * @brief Get member's value.
 *
 */
template <std::size_t Index, reflectible Ty>
constexpr auto& get(Ty&& obj) noexcept {
    static_assert(Index < member_count_of<Ty>());
    using pure_t = std::remove_cvref_t<Ty>;

    if constexpr (default_reflectible_aggregate<Ty>) {
        auto tuple = object_to_tuple_view<pure_t>(std::forward<Ty>(obj));
        return std::get<Index>(tuple);
    } else { // has_field_traits
        auto traits = pure_t::field_traits();
        return std::get<Index>(traits).get(obj);
    }
}

} // namespace _reflection

} // namespace neutron
