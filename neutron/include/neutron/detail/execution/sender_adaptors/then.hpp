#pragma once
#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/movable_value.hpp"
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"

namespace neutron::execution {

template <typename Fn>
struct _then_receiver {
    Fn fn;
    template <typename F>
    requires std::constructible_from<Fn, F>
    _then_receiver(F&& fn) : fn(std::forward<F>(fn)) {}
};

template <sender Sndr, typename Fn>
struct _then_sender {
    Fn fn;
    template <typename F>
    requires std::constructible_from<Fn, F>
    _then_sender(F&& fn) : fn(std::forward<F>(fn)) {}
};

struct then_t {
    template <typename Fn>
    constexpr auto operator()(Fn&& fn) const {
        return _sender_adaptor{ *this, std::forward<Fn>(fn) };
    }

    template <sender Sndr, movable_value Fn>
    constexpr auto operator()(Sndr&& sndr, Fn&& fn) const {
        auto domain = _get_domain_early(sndr);
        return transform_sender(
            domain,
            make_sender(*this, std::forward<Fn>(fn), std::forward<Sndr>(sndr)));
    }
};

inline constexpr then_t then;

template <>
struct _impls_for<then_t> : _default_impls {
    static constexpr auto complete =
        []<typename Ignored, typename Tag, typename State, typename Receiver,
           typename... Args>(
            Ignored, State& fn, Receiver& rcvr, Tag, Args&&... args) noexcept {
            // if constexpr (std::same_as<Tag, set_value_t>) {
            ATOM_TRY {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<State&, Args...>>) {
                    std::invoke(fn, std::forward<Args>(args)...);
                    set_value(std::move(rcvr));
                } else {
                    set_value(
                        std::move(rcvr),
                        std::invoke(fn, std::forward<Args>(args)...));
                }
            }
            ATOM_CATCH(...) {
                if constexpr (!std::is_nothrow_invocable_v<State&, Args...>) {
                    set_error(std::move(rcvr), std::current_exception());
                }
            }
            // } else {
            //     Tag()(std::move(rcvr), std::forward<Args>(args)...);
            // }
        };
};

// Completion signatures for then(sender, fn)
// Map each child set_value_t(Args...) to set_value_t(invoke_result_t<Fn&,
// Args...>) (or set_value_t() if the result type is void). Forward
// set_error_t(E) and set_stopped_t() unchanged.

template <typename Fn, typename Sndr, typename Env>
struct _completion_signatures_for_impl<_basic_sender<then_t, Fn, Sndr>, Env> {
    using _child_completions = completion_signatures_of_t<Sndr, Env>;

    template <typename Sig>
    struct _map_sig {
        using type = Sig;
    };

    template <typename Res, bool = std::is_void_v<Res>>
    struct _map_set_value {
        using type = set_value_t(std::decay_t<Res>);
    };

    template <typename Res>
    struct _map_set_value<Res, true> {
        using type = set_value_t();
    };

    template <typename... Args>
    struct _map_sig<set_value_t(Args...)> {
        using _res = std::invoke_result_t<Fn&, Args...>;
        using type = typename _map_set_value<_res>::type;
    };

    template <typename... Sigs>
    struct _transform;

    template <typename... Sigs>
    struct _transform<completion_signatures<Sigs...>> {
        using type = completion_signatures<typename _map_sig<Sigs>::type...>;
    };

    using type = typename _transform<_child_completions>::type;
};

} // namespace neutron::execution
