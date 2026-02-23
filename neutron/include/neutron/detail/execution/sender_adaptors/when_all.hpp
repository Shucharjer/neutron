#pragma once
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <variant>
#include <neutron/metafn.hpp>
#include <neutron/utility.hpp>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/join_env.hpp"
#include "neutron/detail/execution/make_env.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/set_error.hpp"
#include "neutron/detail/execution/set_stopped.hpp"
#include "neutron/detail/utility/none_such.hpp"

namespace neutron::execution {

inline constexpr struct when_all_t {
    template <sender... Senders>
    requires(sizeof...(Senders) != 0)
    constexpr auto operator()(Senders&&... senders) const {
        using common_t =
            std::common_type_t<decltype(_get_domain_early(senders))...>;
        return transform_sender(
            common_t{},
            make_sender(*this, {}, std::forward<Senders>(senders)...));
    }

} when_all{};

namespace _when_all {

template <typename Sndr, typename Env>
concept _max_1_sender_in =
    sender_in<Sndr, Env> && // exposition only
    (std::tuple_size_v<value_types_of_t<Sndr, Env, std::tuple, std::tuple>> <=
     1);

}

template <>
struct _impls_for<when_all_t> : _default_impls {
    static constexpr struct _get_attrs_impl {
        constexpr auto operator()(auto&&, auto&&... child) noexcept {
            using common_t =
                std::common_type_t<decltype(_get_domain_early(child))...>;

            if constexpr (std::same_as<common_t, default_domain>) {
                return empty_env{};
            } else {
                return _make_env(get_domain, common_t{});
            }
        }
    } get_attrs{};

    static constexpr struct _get_env_impl {
        template <typename State, typename Rcvr>
        constexpr auto
            operator()(auto&&, State& state, const Rcvr& rcvr) noexcept {
            return _join_env(
                make_env(get_stop_token, state.stop_src.get_token()),
                get_env(rcvr));
        }
    } get_env{};

    template <typename Rcvr, typename... Sndrs>
    struct _state_type {
        void complete(Rcvr& rcvr) noexcept {}
    };

    enum class _disposition // exposition only
        : uint8_t /* underlying not required*/ {
        started,
        error,
        stopped
    };

    template <typename Rcvr>
    struct _make_state {
        template <_when_all::_max_1_sender_in<env_of_t<Rcvr>>... Sndrs>
        constexpr auto operator()(auto, auto, Sndrs&&... sndrs) const {
            // NOTE: if this type is well-formed; otherwise, tuple<>
            using values_tuple = std::tuple<value_types_of_t<
                Sndrs, env_of_t<Rcvr>, _decayed_tuple, std::optional>...>;

            using errors_variant = type_list_cat_t<
                std::variant<none_such, std::exception_ptr>,
                type_list_rebind_t<
                    std::variant,
                    type_list_cat_t<error_types_of_t<
                        Sndrs, env_of_t<Rcvr>, decayed_type_list>...>>>;

            using stop_callback = stop_callback_of_t<
                stop_token_of_t<env_of_t<Rcvr>>, _on_stop_request>;

            struct _state_type {
                void arrice(Rcvr& rcvr) noexcept {
                    if (0 == --count) {
                        complete(rcvr);
                    }
                }

                void complete(Rcvr& rcvr) noexcept;

                std::atomic<size_t> count{ sizeof...(sndrs) };
                inplace_stop_source stop_src{};
                std::atomic<_disposition> disp{ _disposition::started };
                errors_variant errors{};
                values_tuple values{};
                std::optional<stop_callback> on_stop{ nullopt };
            };
            return _state_type{};
        }
    };

    static constexpr struct _get_state_impl {
        // NOTE: apply is a exposition only function
        template <typename Sndr, typename Rcvr>
        constexpr auto operator()(Sndr&& sndr, Rcvr& rcvr) noexcept(
            noexcept(std::forward<Sndr>(sndr).apply(_make_state<Rcvr>()))) {
            return std::forward<Sndr>(sndr).apply(_make_state<Rcvr>());
        }
    } get_state{};

    static constexpr struct _start_impl {
    } start{};

    static constexpr struct _complete_impl {
        template <
            typename Index, typename State, typename Rcvr, typename Set,
            typename... Args>
        constexpr void operator()(
            Index, State& state, Rcvr& rcvr, Set, Args&&... args) noexcept {
            if constexpr (std::same_as<Set, set_error_t>) {

            } else if constexpr (std::same_as<Set, set_stopped_t>) {

            } else if constexpr (!std::same_as<
                                     decltype(State::values), std::tuple<>>) {
            }
            state.arrive(rcvr);
        }
    } complete{};
};

} // namespace neutron::execution
