#pragma once
#include "./fwd.hpp"

// NOTE: tag_invoke lives under concepts/, not utility/
#include "../concepts/tag_invoke.hpp"

namespace neutron::execution {

template <typename EnvProvider>
concept _has_member_get_env = requires(EnvProvider& ep) { ep.get_env(); };

constexpr inline struct get_env_t {
    template <typename EnvProvider>
    requires _has_member_get_env<EnvProvider>
    constexpr auto operator()(const EnvProvider& ep) const {
        return ep.get_env();
    }

    template <typename EnvProvider>
    requires(!_has_member_get_env<EnvProvider>) &&
            tag_invocable<get_env_t, EnvProvider>
    constexpr auto operator()(const EnvProvider& ep) const {
        return tag_invoke(get_env_t{}, ep);
    }
} get_env{};

template <typename EnvProvider>
using env_of_t = decltype(get_env(std::declval<EnvProvider>()));

template <typename EnvProvider>
concept environment_provider =
    requires(const std::remove_cvref_t<EnvProvider>& ep) {
        { get_env(ep) } -> queryable;
    };

} // namespace neutron::execution
