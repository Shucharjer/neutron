#pragma once
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/queryable_utilities/env.hpp"

namespace neutron::execution {

struct get_env_t {
    template <typename EnvProvider>
    requires requires(const std::remove_cvref_t<EnvProvider>& ep) {
        { ep.get_env() } noexcept -> queryable;
    }
    auto operator()(EnvProvider&& ep) const noexcept {
        return std::as_const(ep).get_env();
    }

    template <typename EnvProvider>
    requires(!requires(const std::remove_cvref_t<EnvProvider>& ep) {
        { ep.get_env() } noexcept -> queryable;
    })
    auto operator()(EnvProvider&& ep) const noexcept {
        return env<>{};
    }
};

inline constexpr get_env_t get_env;

} // namespace neutron::execution
