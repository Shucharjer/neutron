#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/senders/default_domain.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

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
    auto transformed = try_transform(dom, std::forward<Sndr>(sndr), env...);
    using transformed_sndr_t = decltype(transformed);

    if constexpr (std::same_as<
                      std::remove_cvref_t<transformed_sndr_t>,
                      std::remove_cvref_t<Sndr>>) {
        return transformed;
    } else {
        return impl<Depth + 1>(dom, std::move(transformed), env...);
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

} // namespace neutron::execution
