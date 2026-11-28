// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
#endif
#include "neutron/detail/ecs/concepts/bundle.hpp"
#include "neutron/detail/metafn/requires.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT struct component_tag {};

ATOM_EXPORT template <typename Ty>
constexpr bool as_component = false;

ATOM_EXPORT struct gpu_component_tag : component_tag {};

ATOM_EXPORT template <typename Ty>
constexpr bool as_gpu_component = false;

ATOM_EXPORT template <typename Ty>
concept component =
    (requires {
        typename std::remove_cvref_t<Ty>::component_concept;
        requires std::derived_from<
            typename std::remove_cvref_t<Ty>::component_concept, component_tag>;
    } || as_component<std::remove_cvref_t<Ty>>) &&
    std::default_initializable<std::remove_cvref_t<Ty>> &&
    std::movable<std::remove_cvref_t<Ty>> &&
    std::destructible<std::remove_cvref_t<Ty>> &&
    std::is_nothrow_destructible_v<Ty>;

ATOM_EXPORT template <typename Ty>
concept gpu_component =
    component<Ty> && (std::derived_from<
                          typename std::remove_cvref_t<Ty>::component_concept,
                          gpu_component_tag> ||
                      as_gpu_component<Ty>);

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename Ty>
struct _is_component {
    constexpr static bool value = component<Ty>;
};

} // namespace internal
/* @endcond */

ATOM_EXPORT template <typename Ty>
concept component_like =
    component<Ty> ||
    (bundled<Ty> && type_list_requires_recurse<
                        internal::_is_component, Ty, internal::_is_bundle,
                        std::remove_cvref_t>::value);

} // namespace neutron
