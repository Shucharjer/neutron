#pragma once
#include <tuple>
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/product_type.hpp"
#include "neutron/detail/execution/set_value.hpp"


namespace neutron::execution {

struct just_t {
    template <typename... Args>
    auto operator()(Args&&... args) const noexcept(
        (std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...)) {
        return make_sender(*this, _product_type{ std::forward<Args>(args)... });
    }
};

template <typename... T, typename Env>
struct _completion_signatures_for_impl<
    _basic_sender<just_t, _product_type<T...>>, Env> {
    using type = completion_signatures<set_value_t(T...)>;
};

inline constexpr just_t just;

template <>
struct _impls_for<just_t> : _default_impls {
    static constexpr auto start =
        []<typename State>(State& state, auto& rcvr) noexcept -> void {
        std::apply(
            [&rcvr](auto&... vals) mutable {
                set_value(std::move(rcvr), vals...);
            },
            state);
    };
};

} // namespace neutron::execution
