#pragma once
#include "./fwd.hpp"

#include "../pipeline.hpp"
#include "./sender.hpp"

namespace neutron::execution {

template <typename Derived>
struct sender_adaptor_closure : adaptor_closure<Derived> {};

template <typename Ty>
concept _sender_adaptor_closure = _adaptor_closure<Ty, sender_adaptor_closure>;

template <typename Ty, typename Sender>
concept _sender_adaptor_closure_for =
    sender<std::remove_cvref_t<Sender>> &&
    _adaptor_closure_for<Ty, sender_adaptor_closure, Sender> &&
    sender<std::invoke_result_t<Ty, std::remove_cvref_t<Ty>>>;

template <typename Closure1, typename Closure2>
struct _sender_closure_compose :
    public _closure_compose<Closure1, Closure2, sender_adaptor_closure> {
    using _compose_base =
        _closure_compose<Closure1, Closure2, sender_adaptor_closure>;

    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
_sender_closure_compose(C1&&, C2&&) -> _sender_closure_compose<
    std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <typename Fn>
class _sender_closure : public sender_adaptor_closure<_sender_closure<Fn>> {
public:
    template <typename F>
    requires std::constructible_from<Fn, F>
    constexpr _sender_closure(F&& fn) noexcept : fn_(std::forward<F>(fn)) {}

    template <sender Sndr>
    constexpr auto operator()(Sndr&& sndr) const {
        return fn_(std::forward<Sndr>(sndr));
    }

private:
    Fn fn_;
};

template <typename Fn>
_sender_closure(Fn&&) -> _sender_closure<std::remove_cvref_t<Fn>>;

} // namespace neutron::execution

template <
    neutron::execution::sender Sender,
    neutron::execution::_sender_adaptor_closure_for<Sender> Closure>
constexpr decltype(auto) operator|(Sender&& sender, Closure&& closure) noexcept(
    std::is_nothrow_invocable_v<Closure, Sender>) {
    return std::forward<Closure>(closure)(std::forward<Sender>(sender));
}

template <
    neutron::execution::_sender_adaptor_closure C1,
    neutron::execution::_sender_adaptor_closure C2>
constexpr auto operator|(C1&& closure1, C2&& closure2)
    -> neutron::execution::_sender_closure_compose<
        std::remove_cvref_t<C1>, std::remove_cvref_t<C2>> {
    return _sender_closure_compose(
        std::forward<C1>(closure1), std::forward<C2>(closure2));
}
