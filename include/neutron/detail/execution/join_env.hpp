#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp" // IWYU pragma: keep

namespace neutron::execution {

template <typename Env1, typename Env2>
class _join_env {
private:
    Env1 env1_;
    Env2 env2_;

public:
    template <typename E1, typename E2>
    _join_env(E1&& e1, E2&& e2)
        : env1_(::std::forward<E1>(e1)), env2_(::std::forward<E2>(e2)) {}

    template <typename Query, typename... Args>
    requires(
        requires(Env1&, const Query& query, Args&&... args) {
            env1_.query(query, ::std::forward<Args>(args)...);
        } ||
        requires(Env2& e2, const Query& query, Args&&... args) {
            e2.query(query, ::std::forward<Args>(args)...);
        })
    auto query(const Query& query, Args&&... args) noexcept -> decltype(auto) {
        if constexpr (requires {
                          env1_.query(query, ::std::forward<Args>(args)...);
                      }) {
            return env1_.query(query, ::std::forward<Args>(args)...);
        } else {
            return env2_.query(query, ::std::forward<Args>(args)...);
        }
    }
    template <typename Query, typename... Args>
    requires(
        requires(const Env1&, const Query& query, Args&&... args) {
            env1_.query(query, ::std::forward<Args>(args)...);
        } ||
        requires(const Env2& e2, const Query& query, Args&&... args) {
            e2.query(query, ::std::forward<Args>(args)...);
        })
    auto query(const Query& query, Args&&... args) const noexcept
        -> decltype(auto) {
        if constexpr (requires {
                          env1_.query(query, ::std::forward<Args>(args)...);
                      }) {
            return env1_.query(query, ::std::forward<Args>(args)...);
        } else {
            return env2_.query(query, ::std::forward<Args>(args)...);
        }
    }
};

template <typename Env1, typename Env2>
_join_env(Env1&&, Env2&&)
    -> _join_env<::std::remove_cvref_t<Env1>, ::std::remove_cvref_t<Env2>>;

} // namespace neutron::execution
