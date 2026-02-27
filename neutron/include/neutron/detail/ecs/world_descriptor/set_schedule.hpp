// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <neutron/concepts.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

template <double Freq>
struct frequency {};

template <typename T>
constexpr bool _is_group_desc = false;
template <>
constexpr bool _is_group_desc<individual> = true;
template <size_t Index>
constexpr bool _is_group_desc<group<Index>> = true;

template <typename Policy>
concept _schedule_policy =
    is_specific_value_list_v<frequency, Policy> || _is_group_desc<Policy>;

template <_schedule_policy... Policy>
struct _set_scheduler_t :
    descriptor_adaptor_closure<_set_scheduler_t<Policy...>> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor descriptor) const noexcept {
        return typename Descriptor::template set_schedule_t<Policy...>{};
    }
};
;

template <_schedule_policy... Policy>
inline constexpr _set_scheduler_t<Policy...> set_schedule{};

} // namespace neutron
