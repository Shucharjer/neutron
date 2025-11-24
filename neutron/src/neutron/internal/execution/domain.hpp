#pragma once
#include "./fwd.hpp"

#include "./sender.hpp"
#include "../get.hpp"

namespace neutron::execution {

/// Let `sndr` be an expression such that `decltype((sndr))` is `Sndr`.
/// The type `tag_of_t<Sndr>` is as follows:
/// - If the declaration `auto&& [tag, data, ...children] = sndr;` would be
/// well-formed, `tag_of_t<Sndr>` is an alias for `decltype(auto(tag))`.
/// - Otherwise, `tag_of_t<Sndr>` is ill-formed.
template <sender Sndr>
requires gettible<Sndr, 0>
using tag_of_t = decltype(get<0>(std::declval<Sndr>()));

template <typename Sndr, typename... Args>
concept _has_member_transform_sender =
    sender<Sndr> && requires(Sndr&& sndr, Args&&... args) {
        tag_of_t<Sndr>().transform_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    };

template <typename Sndr, typename... Args>
concept _has_member_transform_env =
    sender<Sndr> && requires(Sndr&& sndr, Args&&... args) {
        tag_of_t<Sndr>().transform_env(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    };

struct default_domain {
    template <sender Sndr, queryable... Env>
    requires(sizeof...(Env) <= 1)
    constexpr sender decltype(auto)
        transform_sender(Sndr&& sndr, const Env&... env) noexcept(
            _has_member_transform_sender<Sndr, const Env&...>
                ? noexcept(tag_of_t<Sndr>().transform_sender(
                      std::forward<Sndr>(sndr), env...))
                : true) {
        if constexpr (_has_member_transform_sender<Sndr, const Env&...>) {
            return tag_of_t<Sndr>().transform_sender(
                std::forward<Sndr>(sndr), env...);
        } else {
            return std::forward<Sndr>(sndr);
        }
    }

    template <sender Sndr, queryable Env>
    constexpr queryable decltype(auto)
        transform_env(Sndr&& sndr, Env&& env) noexcept(
            _has_member_transform_env<Sndr, Env&&>
                ? noexcept(tag_of_t<Sndr>().transform_env(
                      std::forward<Sndr>(sndr), std::forward<Env>(env)))
                : true) {
        if constexpr (_has_member_transform_env<Sndr, Env&&>) {
            return tag_of_t<Sndr>().transform_env(
                std::forward<Sndr>(sndr), std::forward<Env>(env));
        } else {
            return static_cast<Env>(std::forward<Env>(env));
        }
    }

    template <typename Tag, sender Sndr, typename... Args>
    static constexpr decltype(auto)
        apply_sender(Tag, Sndr&& sndr, Args&&... args) noexcept(
            noexcept(Tag().apply_sender(
                std::forward<Sndr>(sndr), std::forward<Args>(args)...))) {
        Tag().apply_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
};

} // namespace neutron::execution
