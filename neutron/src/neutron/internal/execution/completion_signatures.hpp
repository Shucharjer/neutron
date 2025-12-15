#pragma once
#include "./fwd.hpp"

#include <tuple>
#include <type_traits>
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
    constexpr auto operator()(Sndr&& sndr, Env&& env) const noexcept {
        constexpr auto mksndr = [](auto&& sndr, auto&& env) {
            auto domain = _get_late_domain(sndr, env);
            return transform_sender(
                domain, std::forward<decltype(sndr)>(sndr),
                std::forward<decltype(env)>(env));
        };

        using sndr_t = std::remove_cvref_t<decltype(mksndr(sndr, env))>;
        if constexpr (requires { // has member get_completion_signatures
                          mksndr(sndr, env).get_completion_signatures();
                      }) {
            return decltype(mksndr(sndr, env).get_completion_signatures()){};
            // } else if constexpr (tag_invocable<get_completion_signatures_t,
            // >) {
        }
    }
} get_completion_signatures;

template <typename Sndr, typename Env>
using completion_signatures_of_t = decltype(get_completion_signatures(
    std::declval<Sndr>(), std::declval<Env>()));

template <typename... Args>
using _decayed_tuple = std::tuple<std::decay_t<Args>...>;

template <typename Tag, typename Fn>
struct _same_with_tag : std::false_type {};
template <typename Tag, typename Ret, typename... Args>
struct _same_with_tag<Tag, Ret(Args...)> : std::is_same<Tag, Ret> {};

template <
    typename Tag, typename Sigs, template <typename...> typename Tuple,
    template <typename...> typename Variant>
struct _gather_signatures_helper {
    template <typename Ty>
    using _same_with_tag = std::is_same<Tag, Ty>;
};

template <
    typename Tag, typename Signatures, template <typename...> typename Tuple,
    template <typename...> typename Variant>
using _gather_signatures =
    typename _gather_signatures_helper<Tag, Signatures, Tuple, Variant>::type;

template <
    typename Sender, typename Env = empty_env,
    template <typename...> typename Tuple   = _decayed_tuple,
    template <typename...> typename Variant = std::type_identity_t>
using value_types_of_t = _gather_signatures<
    set_value_t, completion_signatures_of_t<Sender, Env>, Tuple, Variant>;

template <
    typename Sender, typename Env = empty_env,
    template <typename...> typename Tuple   = _decayed_tuple,
    template <typename...> typename Variant = std::type_identity_t>
using error_types_of_t = _gather_signatures<
    set_error_t, completion_signatures_of_t<Sender, Env>, Tuple, Variant>;

} // namespace neutron::execution
