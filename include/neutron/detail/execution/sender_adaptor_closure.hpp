#pragma once
#include <type_traits>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/pipeline.hpp"

namespace neutron::execution {

template <typename Derived>
struct sender_adaptor_closure : adaptor_closure<Derived> {};

template <typename Ty>
concept _sender_adaptor_closure = _adaptor_closure<Ty, sender_adaptor_closure>;

template <typename First, typename Second>
struct _compose :
    public _closure_compose<_compose, sender_adaptor_closure, First, Second> {
    using _compose_base =
        _closure_compose<_compose, sender_adaptor_closure, First, Second>;
    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
_compose(C1&&, C2&&) -> _compose<std::decay_t<C1>, std::decay_t<C2>>;

template <typename Closure, typename Sender>
concept _sender_adaptor_closure_for =
    _sender_adaptor_closure<Closure> && sender<Sender> &&
    std::invocable<Closure, std::decay_t<Sender>> &&
    sender<std::invoke_result_t<Closure, std::decay_t<Sender>>>;

template <sender Sndr, _sender_adaptor_closure_for<Sndr> Closure>
constexpr decltype(auto) operator|(Sndr&& sndr, Closure&& closure) {
    return std::forward<Closure>(closure)(std::forward<Sndr>(sndr));
}

template <_sender_adaptor_closure Closure1, _sender_adaptor_closure Closure2>
constexpr decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return _compose(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

} // namespace neutron::execution
