#pragma once
#include "./fwd.hpp"

#include <utility>
#include "../tag_invoke.hpp"

namespace neutron::execution {

template <typename... Sigs>
struct completion_signatures {};

using _success = int;

template <typename CompSigs>
concept valid_completion_signatures = true;

constexpr inline struct set_value_t {

    template <typename Rcvr, typename... Args>
    constexpr static bool _has_member_set_value = requires {
        {
            std::declval<Rcvr>().set_value(std::declval<Args>()...)
        } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr, typename... Args>
    requires _has_member_set_value<Rcvr, Args...>
    constexpr void operator()(Rcvr&& rcvr, Args&&... args) const noexcept {
        std::forward<Rcvr>(rcvr).set_value(std::forward<Args>(args)...);
    }

    template <typename Rcvr, typename... Args>
    requires(!_has_member_set_value<Rcvr, Args...>) &&
            nothrow_tag_invocable<set_value_t, Rcvr, Args...>
    constexpr void operator()(Rcvr&& rcvr, Args&&... args) const noexcept {
        tag_invoke(
            set_value_t{}, std::forward<Rcvr>(rcvr),
            std::forward<Args>(args)...);
    }
} set_value;

constexpr inline struct set_error_t {

    template <typename Rcvr, typename Err>
    constexpr static bool _has_member_set_error = requires {
        {
            std::declval<Rcvr>().set_error(std::declval<Err>())
        } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr, typename Err>
    requires _has_member_set_error<Rcvr, Err>
    constexpr void operator()(Rcvr&& rcvr, Err&& err) const noexcept {
        std::forward<Rcvr>(rcvr).set_error(std::forward<Err>(err));
    }

    template <typename Rcvr, typename Err>
    requires(!_has_member_set_error<Rcvr, Err>) &&
            nothrow_tag_invocable<set_error_t, Rcvr, Err>
    constexpr void operator()(Rcvr&& rcvr, Err&& err) const noexcept {
        tag_invoke(
            set_error_t{}, std::forward<Rcvr>(rcvr), std::forward<Err>(err));
    }
} set_error;

constexpr inline struct set_stopped_t {
    template <typename Rcvr>
    constexpr static bool _has_member_set_stopped = requires {
        { std::declval<Rcvr>().set_stopped() } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr>
    requires _has_member_set_stopped<Rcvr>
    constexpr decltype(auto) operator()(Rcvr&& rcvr) const noexcept {
        std::forward<Rcvr>(rcvr).set_stopped();
    }

    template <typename Rcvr>
    requires(!_has_member_set_stopped<Rcvr>) &&
            nothrow_tag_invocable<set_stopped_t, Rcvr>
    constexpr decltype(auto) operator()(Rcvr&& rcvr) const noexcept {
        tag_invoke(set_stopped_t{}, std::forward<Rcvr>(rcvr));
    }
} set_stopped;

constexpr inline struct get_completion_signatures_t {
    template <typename Sndr, typename Env>
    constexpr auto operator()(Sndr&& sndr, Env&& env) const;
} get_completion_signatures;

template <typename Sndr, typename Env>
using completion_signatures_of_t = decltype(get_completion_signatures(
    std::declval<Sndr>(), std::declval<Env>()));

} // namespace neutron::execution
