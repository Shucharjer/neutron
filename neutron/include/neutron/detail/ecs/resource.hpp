// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/metafn/requires.hpp"

namespace neutron {

struct resource_t {};

template <typename Ty>
constexpr bool as_resource = false;

template <typename Ty>
concept resource = requires {
    typename std::remove_cvref_t<Ty>::resource_concept;
    requires std::derived_from<
        typename std::remove_cvref_t<Ty>::resource_concept, resource_t>;
} || as_resource<std::remove_cvref_t<Ty>>;

/*! @cond TURN_OFF_DOXYGEN */
namespace _resource {

template <typename Ty>
struct _is_resource {
    constexpr static bool value = resource<Ty>;
};

} // namespace _resource
/* @endcond */

template <typename Ty>
concept resource_like =
    resource<Ty> ||
    (bundled<Ty> && neutron::type_list_requires_recurse<
                        _resource::_is_resource, Ty, internal::_is_bundle,
                        std::remove_cvref_t>::value);

} // namespace neutron
