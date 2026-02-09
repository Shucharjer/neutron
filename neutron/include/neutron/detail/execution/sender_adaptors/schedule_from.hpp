#pragma once
#include <concepts>
#include <exception>
#include <tuple>
#include <type_traits>
#include <variant>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/query_with_default.hpp"
#include "neutron/detail/execution/sched_attrs.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/utility/fake_copy.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron::execution {

inline constexpr struct schedule_from_t {
    template <scheduler Sch, sender Sndr>
    constexpr auto operator()(Sch&& sch, Sndr&& sndr) const {
        auto domain = _query_with_default(get_domain, sch, default_domain{});
        return transform_sender(
            domain,
            make_sender(
                *this, std::forward<Sch>(sch), std::forward<Sndr>(sndr)));
    }
} schedule_from{};

template <>
struct _impls_for<schedule_from_t> : _default_impls {

    static constexpr auto get_attrs =
        [](const auto& data, const auto& child) noexcept -> decltype(auto) {
        return _join_env(_sched_attrs(data), _fwd_env(get_env(child)));
    };

    template <typename Rcvr, typename Variant>
    struct _state_base {
        Rcvr rcvr;
        Variant async_result{};
    };

    template <typename State>
    struct _receiver_t {
        using receiver_concept = receiver_t;

        State* state;

        constexpr void set_value() && noexcept {
            std::visit(
                [this]<typename Tup>(Tup& result) noexcept -> void {
                    if constexpr (!std::same_as<std::monostate, Tup>) {
                        std::apply(
                            [this](auto& tag, auto&... args) {
                                tag(std::move(state->rcvr), std::move(args)...);
                            },
                            result);
                    }
                },
                state->async_result);
        }

        template <class Error>
        constexpr void set_error(Error&& err) && noexcept {
            execution::set_error(
                std::move(state->rcvr), std::forward<Error>(err));
        }

        constexpr void set_stopped() && noexcept {
            execution::set_stopped(std::move(state->rcvr));
        }

        constexpr decltype(auto) get_env() const noexcept {
            return _fwd_env(execution::get_env(state->rcvr));
        }
    };

    template <typename Rcvr, typename Sch, typename Variant>
    struct _state_type : _state_base<Rcvr, Variant> {
        using receiver_type = _receiver_t<_state_base<Rcvr, Variant>>;
        using operation_type =
            connect_result_t<schedule_result_t<Sch>, receiver_type>;

        operation_type opstate;

        explicit constexpr _state_type(Sch& sch, Rcvr& rcvr)
            : _state_base<Rcvr, Variant>{ rcvr },
              opstate(connect(schedule(sch)), receiver_type{ this }) {}
    };

    static constexpr auto get_state =
        []<typename Sndr, typename Rcvr>(Sndr&& sndr, Rcvr&& rcvr)
    requires sender_in<
        _child_type<Sndr>, decltype(_fwd_env(std::declval<env_of_t<Rcvr>>()))>
    {
        auto& sch       = get<1>(sndr);
        using sched_t   = std::remove_pointer_t<decltype(fake_copy(sch))>();
        using variant_t = type_list_cat_t<
            std::variant<std::monostate>,
            type_list_rebind_t<
                std::variant,
                type_list_cat_t<
                    completion_signatures_of_t<
                        _child_type<Sndr>, env_of_t<Rcvr>>,
                    completion_signatures<
                        set_error_t(std::exception_ptr), set_stopped_t()>>>>;
        return _state_type<Rcvr, sched_t, variant_t>(sch, rcvr);
    };

    static constexpr auto complete = []<class Tag, class... Args>(
                                         auto, auto& state, auto& rcvr, Tag,
                                         Args&&... args) noexcept -> void {
        using result_t = _decayed_tuple<Tag, Args...>;
        constexpr bool nothrow =
            (std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...);

        ATOM_TRY {
            state.async_result.template emplace<result_t>(
                Tag(), std::forward<Args>(args)...);
        }
        ATOM_CATCH(...) {
            if constexpr (!nothrow) {
                state.async_result.template emplace<
                    std::tuple<set_error_t, std::exception_ptr>>(
                    set_error, current_exception());
            }
        }
        start(state.opstate);
    };

    template <typename Sndr, typename... Env>
    static consteval void check_types();
};

} // namespace neutron::execution
