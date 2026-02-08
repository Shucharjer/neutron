#pragma once
#include <utility>
#include "neutron/detail/execution/queries.hpp"

namespace neutron::execution {

template <typename Env>
class _fwd_env {
private:
    Env env_;

public:
    explicit _fwd_env(Env&& env) : env_(::std::forward<Env>(env)) {}

    template <typename Query, typename... Args>
    requires(!forwarding_query(::std::remove_cvref_t<Query>()))
    constexpr auto query(Query&&, Args&&...) const = delete;

    template <typename Query, typename... Args>
    requires(forwarding_query(::std::remove_cvref_t<Query>())) &&
            requires(const Env& env, Query&& query, Args&&... args) {
                env.query(query, ::std::forward<Args>(args)...);
            }
    constexpr auto query(Query&& query, Args&&... args) const
        noexcept(noexcept(env_.query(query, ::std::forward<Args>(args)...))) {
        return env_.query(query, ::std::forward<Args>(args)...);
    }
};
template <typename Env>
_fwd_env(Env&&) -> _fwd_env<Env>;

} // namespace neutron::execution
