// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <type_traits>

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _reflection {

template <typename Ty>
concept aggregate = std::is_aggregate_v<std::remove_cvref_t<Ty>>;

template <typename Ty>
concept has_field_traits =
    requires() { std::remove_cvref_t<Ty>::field_traits(); };

template <typename Ty>
concept has_function_traits =
    requires() { std::remove_cvref_t<Ty>::function_traits(); };

template <typename Ty>
concept default_reflectible_aggregate = aggregate<Ty> && !has_field_traits<Ty>;

template <typename Ty>
concept reflectible = aggregate<Ty> || has_field_traits<Ty>;

} // namespace _reflection
/*! @endcond */

using _reflection::reflectible;

} // namespace neutron
