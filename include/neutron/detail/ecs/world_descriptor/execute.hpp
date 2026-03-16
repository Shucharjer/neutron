// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

namespace _execute {

struct exec_tag_t {};

struct _dynamic_frequency_t {};

template <auto... Policies>
struct _execute_t : descriptor_adaptor_closure<_execute_t<Policies...>> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return insert_range_tagged_value_list_inplace_t<
            exec_tag_t, Descriptor, tagged_value_list, Policies...>{};
    }
};

} // namespace _execute

/**
 * @brief Indicates that the world update interval is configured at runtime.
 *
 * Example tick rates:
 * - 128 tick => runtime interval `1.0 / 128`
 * - 64 tick  => runtime interval `1.0 / 64`
 */
constexpr _execute::_dynamic_frequency_t dynamic_frequency;

template <auto... Policies>
inline constexpr _execute::_execute_t<Policies...> execute;

} // namespace neutron
