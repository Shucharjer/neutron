// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

namespace _execute {

struct _dynamic_frequency_t {};

template <auto... Policies>
struct _execute_t : descriptor_adaptor_closure<_execute_t<Policies...>> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return Descriptor{};
    }
};

} // namespace _execute

// use neutron.individual

// use neutron.group

/**
 * @brief Indicates that the world update with runtime frequency.
 *
 */
constexpr _execute::_dynamic_frequency_t dynamic_frequency;

template <auto... Policies>
inline constexpr _execute::_execute_t<Policies...> execute;

} // namespace neutron
