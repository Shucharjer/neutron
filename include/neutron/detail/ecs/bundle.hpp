// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <typename...>
struct bundle {};

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

template <typename Ty>
concept bundled = internal::_rmcvref_is_bundle<Ty>::value;

} // namespace neutron
