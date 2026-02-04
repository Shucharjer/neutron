#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

    template <typename Fn>
struct _then_receiver {
    Fn fn;
    template <typename F>
    requires std::constructible_from<Fn, F>
    _then_receiver(F&& fn) : fn(std::forward<F>(fn)){}
};

template <sender Sndr, typename Fn>
struct _then_sender {
    Fn fn;
    template <typename F>
    requires std::constructible_from<Fn, F>
    _then_sender(F&& fn) : fn(std::forward<F>(fn)){}
};

struct then_t {
    template <typename Fn>
    auto operator()(Fn&& fn) const {
        return _then_receiver {std::forward<Fn>(fn)};
    }

    template <sender Sndr, typename Fn>
    auto operator()(Sndr&& sndr, Fn&& fn) const {
        return _then_sender{std::forward<Sndr>(sndr), std::forward<Fn>(fn)};
    }
};

inline constexpr then_t then;

}