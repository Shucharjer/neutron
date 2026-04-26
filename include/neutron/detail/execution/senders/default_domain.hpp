#pragma once
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

struct default_domain {
    template <sender Sndr, queryable... Env>
    static constexpr sender decltype(auto)
        transform_sender(Sndr&& sndr, const Env&... env) noexcept(
            noexcept(tag_of_t<Sndr>().transform_sender(
                std::forward<Sndr>(sndr), env...)))
    requires(sizeof...(Env) <= 1) && requires {
        tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...);
    }
    {
        return tag_of_t<Sndr>().transform_sender(
            std::forward<Sndr>(sndr), env...);
    }

    template <sender Sndr, queryable... Env>
    static constexpr sender decltype(auto)
        transform_sender(Sndr&& sndr, const Env&... env) noexcept
    requires(sizeof...(Env) <= 1) && (!requires {
                tag_of_t<Sndr>().transform_sender(
                    std::forward<Sndr>(sndr), env...);
            })
    {
        return std::forward<Sndr>(sndr);
    }

    template <sender Sndr, queryable Env>
    static constexpr queryable decltype(auto)
        transform_env(Sndr&& sndr, Env&& env) noexcept {
        if constexpr (requires {
                          {
                              tag_of_t<Sndr>().transform_env(
                                  std::forward<Sndr>(sndr),
                                  std::forward<Env>(env))
                          } noexcept;
                      }) {
            return tag_of_t<Sndr>().transform_env(
                std::forward<Sndr>(sndr), std::forward<Env>(env));
        } else {
            static_assert(noexcept(static_cast<Env>(std::forward<Env>(env))));
            return static_cast<Env>(std::forward<Env>(env));
        }
    }

    template <typename Tag, sender Sndr, typename... Args>
    static constexpr decltype(auto)
        apply_sender(Tag, Sndr&& sndr, Args&&... args) noexcept(
            noexcept(Tag().apply_sender(
                std::forward<Sndr>(sndr), std::forward<Args>(args)...))) {
        return Tag().apply_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
};

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
        return default_domain{}.transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    }
}


} // namespace neutron::execution
