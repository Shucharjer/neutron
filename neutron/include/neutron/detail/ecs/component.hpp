// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/metafn/requires.hpp"


namespace neutron {

struct component_t {};

template <typename Ty>
constexpr bool as_component = false;

template <typename Ty>
concept component =
    (requires {
        typename std::remove_cvref_t<Ty>::component_concept;
        requires std::derived_from<
            typename std::remove_cvref_t<Ty>::component_concept, component_t>;
    } || as_component<std::remove_cvref_t<Ty>>) &&
    std::default_initializable<std::remove_cvref_t<Ty>> &&
    std::movable<std::remove_cvref_t<Ty>> &&
    std::destructible<std::remove_cvref_t<Ty>> &&
    std::is_nothrow_destructible_v<Ty>;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename Ty>
struct _is_component {
    constexpr static bool value = component<Ty>;
};

} // namespace internal
/* @endcond */

template <typename Ty>
concept component_like =
    component<Ty> ||
    (bundled<Ty> && type_list_requires_recurse<
                        internal::_is_component, Ty, internal::_is_bundle,
                        std::remove_cvref_t>::value);

} // namespace neutron
