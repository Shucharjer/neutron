#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace neutron::execution {

template <typename... Args>
struct _product_type {
    std::tuple<Args...> args;

    template <typename... Values>
    constexpr _product_type(Values&&... values) noexcept(
        std::is_nothrow_constructible_v<std::tuple<Args...>, Values...>)
        : args(std::forward<Values>(values)...) {}

    template <size_t I>
    constexpr decltype(auto) get() & noexcept {
        return std::get<I>(args);
    }
    template <size_t I>
    constexpr decltype(auto) get() const& noexcept {
        return std::get<I>(args);
    }
    template <size_t I>
    constexpr decltype(auto) get() && noexcept {
        return std::get<I>(args);
    }
    template <size_t I>
    constexpr decltype(auto) get() const&& noexcept {
        return std::get<I>(args);
    }

    template <typename Fn>
    constexpr decltype(auto) apply(Fn&& fn) & {
        return std::apply(std::forward<Fn>(fn), args);
    }
    template <typename Fn>
    constexpr decltype(auto) apply(Fn&& fn) const& {
        return std::apply(std::forward<Fn>(fn), args);
    }
    template <typename Fn>
    constexpr decltype(auto) apply(Fn&& fn) && {
        return std::apply(std::forward<Fn>(fn), std::move(args));
    }
    template <typename Fn>
    constexpr decltype(auto) apply(Fn&& fn) const&& {
        return std::apply(std::forward<Fn>(fn), std::move(args));
    }
};

template <typename... Args>
_product_type(Args&&...) -> _product_type<std::decay_t<Args>...>;

} // namespace neutron::execution
