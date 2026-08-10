// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <utility>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/metafn/element.hpp"

namespace neutron {

namespace queries {

template <stage Stage>
struct get_systems_t {
    template <typename... Args>
    consteval auto operator()(world_descriptor_t<Args...>) const noexcept {
        using desc_t       = world_descriptor_t<Args...>;
        constexpr auto pos = []<std::size_t... Is>(std::index_sequence<Is...>) {
            constexpr std::size_t num = sizeof...(Args);
            std::size_t idx           = num;
            std::size_t cur           = 0;
            std::ignore =
                ((idx == num && _has_same_template<
                                    _add_systems_t<Stage>,
                                    type_list_element_t<Is, type_list<Args...>>>
                      ? idx = cur
                      : 0,
                  ++cur) &&
                 ...);
            return idx;
        }(std::index_sequence_for<Args...>());
        if constexpr (pos == sizeof...(Args)) {
            return _add_systems_t<Stage>{};
        } else {
            return type_list_element_t<pos, type_list<Args...>>{};
        }
    }
};

template <stage Stage>
constexpr get_systems_t<Stage> get_systems;

inline constexpr struct get_forward_tick_rate_t {
    template <typename... Args>
    consteval double operator()(world_descriptor_t<Args...>) const noexcept {
        using desc_t = world_descriptor_t<Args...>;
        if constexpr (requires { desc_t::_tick_rate; }) {
            return desc_t::_tick_rate;
        } else {
            return 0.0;
        }
    }
} get_forward_tick_rate;

inline constexpr struct get_execution_policy_t {
    struct policy {
        bool is_individual = false;
        std::size_t id     = 0;
    };

    template <typename... Args>
    consteval policy operator()(world_descriptor_t<Args...>) const noexcept {
        using desc_t = world_descriptor_t<Args...>;
        if constexpr (requires { desc_t::_is_individual; }) {
            if constexpr (desc_t::_is_individual) {
                return policy{ .is_individual = true, .id = 0 };
            } else if constexpr (requires { desc_t::_group_id; }) {
                return policy{ .is_individual = false,
                               .id            = desc_t::_group_id };
            } else {
                return policy{ .is_individual = false, .id = 0 };
            }
        } else {
            // No `execute<>` clause was piped through — default to group<0>.
            return policy{ .is_individual = false, .id = 0 };
        }
    }
} get_execution_policy;

template <typename Feature>
struct _get_enabled_feature_t {
    template <typename... Args>
    consteval bool operator()(world_descriptor_t<Args...>) const noexcept {
        return std::derived_from<world_descriptor_t<Args...>, Feature>;
    }
};

inline constexpr _get_enabled_feature_t<_enable_events_t> get_enabled_events;
inline constexpr _get_enabled_feature_t<_enable_render_t> get_enabled_render;

} // namespace queries

using queries::get_systems;
using queries::get_forward_tick_rate;
using queries::get_execution_policy;
using queries::get_enabled_events;
using queries::get_enabled_render;

} // namespace neutron
