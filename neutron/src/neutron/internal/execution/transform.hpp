#pragma once
#include "./fwd.hpp"

#include "./domain.hpp"
#include "./sender.hpp"

namespace neutron::execution {

template <typename Domain, sender Sndr, queryable... Env>
requires(sizeof...(Env) <= 1)
constexpr sender decltype(auto)
    transform_sender(Domain dom, Sndr&& sndr, Env&&... env) noexcept {
    if constexpr (requires {
                      dom.transform_sender(
                          std::forward<Sndr>(sndr), std::forward<Env>(env)...);
                  }) {
        return dom.transform_sender(
            std::forward<Sndr>(sndr), std::forward<Env>(env)...);
    } else {
        return default_domain().transform_sender(
            std::forward<Sndr>(sndr), std::forward<Env>(env)...);
    }
}

template <typename Domain, sender Sndr, queryable Env>
constexpr queryable decltype(auto)
    transform_env(Domain dom, Sndr&& sndr, Env&& env) noexcept {
    if constexpr (requires {
                      dom.transform_env(
                          std::forward<Sndr>(sndr), std::forward<Env>(env));
                  }) {
        return dom.transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    } else {
        return default_domain().transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    }
}

template <typename Domain, typename... Args>
concept _has_apply_sender = requires(Domain dom, Args&&... args) {
    dom.apply_sender(std::forward<Args>(args)...);
};

template <typename Domain, typename Tag, sender Sndr, typename... Args>
constexpr decltype(auto) apply_sender(
    Domain dom, Tag, Sndr&& sndr,
    Args&&... args) noexcept(_has_apply_sender<Domain, Tag, Sndr, Args...>) {
    if constexpr (_has_apply_sender<Domain, Tag, Sndr, Args...>) {
        return dom.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    } else {
        return default_domain().apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
}

} // namespace neutron::execution
