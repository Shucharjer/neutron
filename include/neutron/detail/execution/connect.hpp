#pragma once
#include <concepts>
#include <coroutine>
#include <exception>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/set_value.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

template <typename T>
struct _awaiter_set_value {
    using type = set_value_t(T);
};

template <>
struct _awaiter_set_value<void> {
    using type = set_value_t();
};

template <typename Rcvr>
struct connect_awaitable_promise {};

template <typename Fn, typename... Args>
auto suspend_complete(Fn&& fn, Args&&... args) noexcept {
    auto result = std::forward<Fn>(fn)(std::forward<Args>(args)...);
    struct awaiter {
        std::decay_t<Fn> fn;

        static constexpr auto await_ready() noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<>) noexcept { this->fn(); }

#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
        [[noreturn]] void await_resume() noexcept { std::unreachable(); }
#else
        [[noreturn]] void await_resume() noexcept { std::terminate(); }
#endif
    };
}

template <typename Awaiter, typename Rcvr>
using awaiter_completion_signatures = completion_signatures<
    typename _awaiter_set_value<
        _await_result_type<Awaiter, connect_awaitable_promise<Rcvr>>>::type,
    set_error_t(std::exception_ptr), set_stopped_t()>;

inline constexpr struct connect_t {
    template <typename Awaiter, receiver Rcvr>
    requires receiver_of<Rcvr, awaiter_completion_signatures<Awaiter, Rcvr>>
    static auto connect_awaitable(Awaiter awaiter, Rcvr rcvr) {
        using result_t =
            _await_result_type<Awaiter, connect_awaitable_promise<Rcvr>>;

        std::exception_ptr eptr;
        ATOM_TRY {
            if constexpr (std::same_as<void, result_t>) {
                co_await std::move(awaiter);
                co_await suspend_complete(set_value, std::move(rcvr));
            } else {
                suspend_complete(
                    set_value, std::move(rcvr), co_await std::move(awaiter));
            }
        }
        ATOM_CATCH(...) { eptr = std::current_exception(); }
        co_await suspend_complete(set_error, std::move(rcvr), eptr);
    }

    template <sender Sndr, receiver Rcvr>
    constexpr auto operator()(Sndr&& sndr, Rcvr&& rcvr) const {
        auto make = [&sndr, &rcvr] {
            auto domain = _get_domain_late(sndr, get_env(rcvr));
            return transform_sender(
                domain, std::forward<Sndr>(sndr), get_env(rcvr));
        };

        if constexpr (requires { make().connect(std::forward<Rcvr>(rcvr)); }) {
            return make().connect(std::forward<Rcvr>(rcvr));
        } else if constexpr (requires {
                                 connect_awaitable(
                                     make(), std::forward<Rcvr>(rcvr));
                             }) {
            return connect_awaitable(make, std::forward<Rcvr>(rcvr));
        } else {
            static_assert(
                false,
                "result from transform_sender has no suitable 'connect'");
        }
    }

} connect;

} // namespace neutron::execution
