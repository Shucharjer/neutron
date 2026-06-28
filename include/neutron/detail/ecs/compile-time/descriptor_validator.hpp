#pragma once
#include <neutron/detail/ecs/compile-time/descriptor.hpp>
#include "neutron/detail/ecs/compile-time/queries.hpp"

namespace neutron {

template <descriptor Descriptor>
struct descriptor_validator;

template <typename... Args>
struct descriptor_validator<world_descriptor_t<Args...>> {
    template <stage Stage>
    constexpr bool is_valid() noexcept {
        constexpr auto tasks =
            get_systems<Stage>(world_descriptor_t<Args...>());
        return true;
    }
};

} // namespace neutron
