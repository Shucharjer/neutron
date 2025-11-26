#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
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
    _closure_compose<
        _sender_closure_compose, sender_adaptor_closure, Closure1, Closure2> {
    using _compose_base = _closure_compose<
        _sender_closure_compose, sender_adaptor_closure, Closure1, Closure2>;

    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
_sender_closure_compose(C1&&, C2&&) -> _sender_closure_compose<
    std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <typename... Args>
class _sender_closure :
    public sender_adaptor_closure<_sender_closure<Args...>>,
    public std::tuple<std::decay_t<Args>...> {
    using _base = std::tuple<std::decay_t<Args>...>;

public:
    using _base::_base;

    template <sender Sndr>
    constexpr auto operator()(Sndr&& sndr) {
        return _apply(std::forward<Sndr>(sndr), std::move(*this));
    }

    template <sender Sndr>
    constexpr auto operator()(Sndr&& sndr) const {
        return _apply(std::forward<Sndr>(sndr), *this);
    }

private:
    template <sender Sndr, typename Self>
    constexpr static auto _apply(Sndr&& sndr, Self&& self) {
        return std::apply(
            [&sndr](auto&& fn, auto&&... args) {
                return fn(
                    std::forward<Sndr>(sndr),
                    std::forward<decltype(args)>(args)...);
            },
            std::forward<Self>(self));
    }
};

template <typename Fn>
_sender_closure(Fn&&) -> _sender_closure<std::remove_cvref_t<Fn>>;

} // namespace neutron::execution

template <
    neutron::execution::sender Sender,
    neutron::execution::_sender_adaptor_closure_for<Sender> Adaptor>
constexpr decltype(auto) operator|(Sender&& sender, Adaptor&& adaptor) noexcept(
    std::is_nothrow_invocable_v<Adaptor, Sender>) {
    return std::forward<Adaptor>(adaptor)(std::forward<Sender>(sender));
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

template <typename... Args>
struct std::tuple_size<neutron::execution::_sender_closure<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::execution::_sender_closure<Args...>> :
    std::tuple_element<Index, std::tuple<std::decay_t<Args>...>> {};
