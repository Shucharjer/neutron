#pragma once
#include <concepts>
#include <coroutine>
#include <exception>
#include <tuple>
#include <type_traits>
#include <variant>
#include <neutron/concepts.hpp>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/utility/as_except_ptr.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron::execution {

template <typename Sndr, typename Env>
struct _single_sender_value_type_helper;

template <typename Sndr, typename Env>
requires requires {
    typename value_types_of_t<Sndr, Env, std::decay_t, std::type_identity_t>;
}
struct _single_sender_value_type_helper<Sndr, Env> {
    using type =
        value_types_of_t<Sndr, Env, std::decay_t, std::type_identity_t>;
};

template <typename Sndr, typename Env>
requires one_of<
    value_types_of_t<Sndr, Env, std::tuple, std::variant>,
    std::variant<std::tuple<>>, std::variant<>>
struct _single_sender_value_type_helper<Sndr, Env> {
    using type = void;
};

template <typename Sndr, typename Env>
requires(
    !requires {
        typename value_types_of_t<
            Sndr, Env, std::decay_t, std::type_identity_t>;
    } &&
    !one_of<
        value_types_of_t<Sndr, Env, std::tuple, std::variant>,
        std::variant<std::tuple<>>, std::variant<>>)
struct _single_sender_value_type_helper<Sndr, Env> {
    using type =
        value_types_of_t<Sndr, Env, _decayed_tuple, std::type_identity_t>;
};

template <typename Sndr, typename Env>
using _single_sender_value_type =
    typename _single_sender_value_type_helper<Sndr, Env>::type;

template <typename Sndr, typename Env>
concept _single_sender = sender_in<Sndr, Env> && requires {
    typename _single_sender_value_type<Sndr, Env>;
};

template <class Sndr>
concept _has_queryable_await_completion_adaptor = // exposition only
    sender<Sndr> &&
    requires(Sndr&& sender) { get_await_completion_adaptor(get_env(sender)); };

template <class Sndr, class Promise>
class _sender_awaitable {
    struct unit {};
    using _value_type = _single_sender_value_type<Sndr, env_of_t<Promise>>;
    using _result_type =
        std::conditional_t<std::is_void_v<_value_type>, unit, _value_type>;

    struct _awaitable_receiver {
        using receiver_concept = receiver_t;

        std::variant<std::monostate, _result_type, std::exception_ptr>*
            result_ptr;
        std::coroutine_handle<Promise> continuation;

        template <typename... Vs>
        requires std::constructible_from<_result_type, Vs...>
        void set_value(Vs&&... vs) && noexcept {
            ATOM_TRY {
                result_ptr->template emplace<1>(std::forward<Vs>(vs)...);
            }
            ATOM_CATCH(...) {
                result_ptr->template emplace<2>(std::current_exception());
            }
            continuation.resume();
        }

        template <typename Err>
        void set_error(Err&& err) && noexcept {
            result_ptr->template emplace<2>(
                as_except_ptr(std::forward<Err>(err)));
            continuation.resume();
        }

        void set_stopped() && noexcept {
            static_cast<std::coroutine_handle<>>(
                continuation.promise().unhandled_stopped())
                .resume();
        }

        auto get_env() const noexcept {
            return _fwd_env(
                ::neutron::execution::get_env(get<2>(*result_ptr).promise()));
        }
    };

    std::variant<std::monostate, _result_type, std::exception_ptr> result_;
    connect_result_t<Sndr, _awaitable_receiver> state_;

public:
    _sender_awaitable(Sndr&& sndr, Promise& promise);
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<Promise>) noexcept {
        ::neutron::execution::start(state_);
    }
    _value_type await_resume();
};

template <typename Sndr, typename Promise>
concept _awaitable_sender =
    _single_sender<Sndr, env_of_t<Promise>> &&
    sender_to<
        Sndr, typename _sender_awaitable<Sndr, Promise>::awaitable_receiver> &&
    requires(Promise& promise) {
        {
            promise.unhandled_stopped()
        } -> std::convertible_to<std::coroutine_handle<>>;
    };

inline constexpr struct as_awaitable_t {
    template <typename T, typename Promise>
    auto operator()(T&& t, Promise& promise) const {

    }
} as_awaitable{};

} // namespace neutron::execution
