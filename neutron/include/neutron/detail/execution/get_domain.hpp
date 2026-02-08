#pragma once
#include <utility>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_scheduler.hpp"
#include "neutron/detail/execution/queries.hpp"
#include "neutron/detail/execution/query_with_default.hpp"

namespace neutron::execution {

inline constexpr struct get_domain_t {
    template <typename Env>
    constexpr auto operator()(Env&& env) const noexcept
    requires requires {
        { std::as_const(env).query(*this) } noexcept;
    }
    {
        return std::as_const(env).query(*this);
    }

    constexpr bool query(forwarding_query_t) const noexcept { return true; }
} get_domain{};

template <typename Sndr>
constexpr auto _get_domain_early(const Sndr& sndr) noexcept {
    if constexpr (requires { get_domain(get_env(sndr)); }) {
        return decltype(get_domain(get_env(sndr))){};
    } else if constexpr (requires { _completion_domain(sndr); }) {
        return decltype(_completion_domain(sndr)){};
    } else {
        return default_domain{};
    }
}

template <typename Sndr, typename Env>
constexpr auto _get_domain_late(const Sndr& sndr, const Env& env) noexcept {
    if constexpr (sender_for<Sndr, continues_on_t>) {
        auto sch = get<1>(sndr);
        return _query_with_default(get_domain, sch, default_domain{});
    } else {
        if constexpr (requires { get_domain(get_env(sndr)); }) {
            return get_domain(get_env(sndr));
        } else if constexpr (requires { _completion_domain<void>(sndr); }) {
            return _completion_domain<void>(sndr);
        } else if constexpr (requires { get_domain(env); }) {
            return get_domain(env);
        } else if constexpr (requires { get_domain(get_scheduler(env)); }) {
            return get_domain(get_scheduler(env));
        } else {
            return default_domain{};
        }
    }
}

} // namespace neutron::execution
