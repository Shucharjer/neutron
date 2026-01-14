// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <ranges>
#include <type_traits>
#include "./pair.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */

namespace _map_like {

template <typename Rng>
concept map = std::ranges::range<Rng> && requires {
    typename std::remove_cvref_t<Rng>::key_type;
    typename std::remove_cvref_t<Rng>::mapped_type;
};

template <typename Rng>
concept map_view =
    std::ranges::view<Rng> && pair<std::ranges::range_value_t<Rng>> &&
    std::is_const_v<typename std::ranges::range_value_t<Rng>::first_type>;

} // namespace _map_like

/*! @endcond */

template <typename Rng>
concept map_like = _map_like::map<Rng> || _map_like::map_view<Rng>;

} // namespace neutron
