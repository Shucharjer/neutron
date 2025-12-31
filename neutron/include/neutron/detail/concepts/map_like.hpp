#pragma once
#include <ranges>
#include "./pair.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */

namespace _map_like {

template <typename Rng>
concept map = std::ranges::range<Rng> && requires {
    typename Rng::key_type;
    typename Rng::mapped_type;
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
