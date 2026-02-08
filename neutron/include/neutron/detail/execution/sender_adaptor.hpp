#pragma once
#include <concepts>
#include <tuple>
#include <type_traits>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/sender_adaptor_closure.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron ::execution {

template <typename Tag, typename... Args>
class _sender_adaptor :
    public sender_adaptor_closure<_sender_adaptor<Tag, Args...>> {
public:
    using sender_concept = sender_t;
    template <typename... Values>
    requires std::constructible_from<std::tuple<Args...>, Values...>
    constexpr _sender_adaptor(Tag, Values&&... values) noexcept(
        std::is_nothrow_constructible_v<std::tuple<Args...>, Values...>)
        : args_(std::forward<Values>(values)...) {}

    template <sender Sndr>
    constexpr decltype(auto) operator()(Sndr&& sndr) {
        return std::apply(
            [&sndr](auto&&... values) {
                return Tag{}(std::forward<Sndr>(sndr), std::move(values)...);
            },
            std::move(args_));
    }

private:
    ATOM_NO_UNIQUE_ADDR std::tuple<Args...> args_;
};

template <typename Tag, typename Fn>
_sender_adaptor(Tag, Fn&&) -> _sender_adaptor<Tag, std::decay_t<Fn>>;

} // namespace neutron::execution
