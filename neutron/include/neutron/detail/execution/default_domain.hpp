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
        Tag().apply_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
};

namespace _transform_sender {

template <typename Domain, typename Sndr, typename... Env>
concept _has_valid_transform =
    requires(Domain dom, Sndr&& sndr, const Env&... env) {
        dom.transform_sender(std::forward<Sndr>(sndr), env...);
    };

template <typename Domain, sender Sndr, queryable... Env>
ATOM_FORCE_INLINE constexpr decltype(auto)
    try_transform(Domain dom, Sndr&& sndr, const Env&... env) noexcept([] {
        if constexpr (_has_valid_transform<Domain, Sndr, Env...>) {
            return noexcept(
                dom.transform_sender(std::forward<Sndr>(sndr), env...));
        } else {
            return noexcept(default_domain{}.transform_sender(
                std::forward<Sndr>(sndr), env...));
        }
    }()) {
    if constexpr (_has_valid_transform<Domain, Sndr, Env...>) {
        return dom.transform_sender(std::forward<Sndr>(sndr), env...);
    } else {
        return default_domain{}.transform_sender(
            std::forward<Sndr>(sndr), env...);
    }
}

static constexpr size_t _recurse_transform_limit = 256;

template <size_t Depth, typename Domain, sender Sndr, queryable... Env>
requires(Depth <= _recurse_transform_limit)
ATOM_FORCE_INLINE constexpr decltype(auto)
    impl(Domain dom, Sndr&& sndr, const Env&... env) {
    using transformed_sndr_t =
        decltype(try_transform(dom, std::forward<Sndr>(sndr), env...));

    if constexpr (std::same_as<
                      std::remove_cvref_t<transformed_sndr_t>,
                      std::remove_cvref_t<Sndr>>) {
        return try_transform(dom, std::forward<Sndr>(sndr), env...);
    } else {
        return impl<Depth + 1>(
            dom, try_transform(dom, std::forward<Sndr>(sndr), env...), env...);
    }
}

} // namespace _transform_sender

template <typename Domain, sender Sndr, queryable... Env>
requires(sizeof...(Env) <= 1)
constexpr sender decltype(auto)
    transform_sender(Domain dom, Sndr&& sndr, const Env&... env) noexcept(
        noexcept(_transform_sender::impl<0>(
            dom, std::forward<Sndr>(sndr), env...))) {
    return _transform_sender::impl<0>(dom, std::forward<Sndr>(sndr), env...);
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
        return default_domain{}.transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    }
}

template <typename Domain, typename Tag, sender Sndr, typename... Args>
constexpr decltype(auto)
    apply_sender(Domain dom, Tag, Sndr&& sndr, Args&&... args) noexcept([] {
        if constexpr (requires {
                          dom.apply_sender(
                              Tag{}, std::forward<Sndr>(sndr),
                              std::forward<Args>(args)...);
                      }) {
            return noexcept(dom.apply_sender(
                Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...));
        } else {
            return noexcept(default_domain{}.apply_sender(
                Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...));
        }
    }()) {
    if constexpr (requires {
                      dom.apply_sender(
                          Tag{}, std::forward<Sndr>(sndr),
                          std::forward<Args>(args)...);
                  }) {
        return dom.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    } else {
        return default_domain{}.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
}

} // namespace neutron::execution
