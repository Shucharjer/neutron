#pragma once
#include "../fwd.hpp"

#include "../sender.hpp"
#include "../sender_adaptor_closure.hpp"
#include "../transform.hpp"

namespace neutron::execution {

constexpr inline struct then_t {
    template <sender Sndr, typename Fn>
    constexpr auto operator()(Sndr&& sndr, Fn&& fn) const;

    template <typename Fn>
    constexpr auto operator()(Fn&& fn) const;
} then;

template <sender Sndr, typename Fn>
constexpr auto then_t::operator()(Sndr&& sndr, Fn&& fn) const {
    auto domain = _get_early_domain(sndr);
    return transform_sender(domain, _make_sender<then_t>(std::forward<Fn&&>(fn), std::forward<Sndr>(sndr)));
}

template <typename Fn>
constexpr auto then_t::operator()(Fn&& fn) const {
    return _sender_closure(std::forward<Fn>(fn));
}

} // namespace neutron::execution
