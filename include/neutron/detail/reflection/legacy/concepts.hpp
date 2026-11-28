// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _refl_legacy {

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

ATOM_EXPORT template <typename Ty>
concept reflectible = aggregate<Ty> || has_field_traits<Ty>;

} // namespace _refl_legacy
/*! @endcond */

ATOM_EXPORT using _refl_legacy::reflectible;

} // namespace neutron
