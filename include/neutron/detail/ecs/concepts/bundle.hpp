// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename...>
struct bundle;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename>
struct _is_bundle : std::false_type {};
template <typename... Args>
struct _is_bundle<bundle<Args...>> : std::true_type {};

template <typename Ty>
struct _rmcvref_is_bundle : _is_bundle<std::remove_cvref_t<Ty>> {};

} // namespace internal
/* @endcond */

ATOM_EXPORT template <typename Ty>
concept bundled = internal::_rmcvref_is_bundle<Ty>::value;

} // namespace neutron
