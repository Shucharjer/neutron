#pragma once
#include <cstddef>
#include <type_traits>
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
    static constexpr struct _start_impl {
        template <typename State>
        constexpr void operator()(State& state, auto& rcvr) const noexcept {
            [&state, &rcvr]<size_t... Is>(std::index_sequence<Is...>) {
                set_value(std::move(rcvr), std::move(get<Is>(state))...);
            }(std::make_index_sequence<
                type_list_size_v<std::remove_cvref_t<State>>>());
        }
    } start{};
};

} // namespace neutron::execution
