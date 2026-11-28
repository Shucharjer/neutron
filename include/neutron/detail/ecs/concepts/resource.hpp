// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
#endif
#include "neutron/detail/ecs/concepts/bundle.hpp"
#include "neutron/detail/metafn/requires.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT struct resource_tag {};

ATOM_EXPORT template <typename Ty>
constexpr bool as_resource = false;

ATOM_EXPORT template <typename Ty>
concept resource = requires {
    typename std::remove_cvref_t<Ty>::resource_concept;
    requires std::derived_from<
        typename std::remove_cvref_t<Ty>::resource_concept, resource_tag>;
} || as_resource<std::remove_cvref_t<Ty>>;

/*! @cond TURN_OFF_DOXYGEN */
namespace _resource {

template <typename Ty>
struct _is_resource {
    constexpr static bool value = resource<Ty>;
};

} // namespace _resource
/* @endcond */

ATOM_EXPORT template <typename Ty>
concept resource_like =
    resource<Ty> ||
    (bundled<Ty> && neutron::type_list_requires_recurse<
                        _resource::_is_resource, Ty, internal::_is_bundle,
                        std::remove_cvref_t>::value);

} // namespace neutron
