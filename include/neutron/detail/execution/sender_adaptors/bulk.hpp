#pragma once
#include <concepts>
#include <exception>
#include <execution>
#include <tuple>
#include <type_traits>
#include "neutron/detail/concepts/movable_value.hpp"
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/product_type.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"
#include "neutron/detail/execution/set_error.hpp"
#include "neutron/detail/execution/set_value.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

inline constexpr struct bulk_t {
    template <typename Shape, typename Fn>
    [[deprecated("bulk should be used with policy")]] constexpr decltype(auto)
        operator()(Shape&& shape, Fn&& fn) const {
        return _sender_adaptor(*this, shape, std::forward<Fn>(fn));
    }

    template <sender Sndr, std::integral Shape, movable_value Fn>
    [[deprecated("bulk should be used with policy")]] constexpr decltype(auto)
        operator()(Sndr&& sndr, Shape&& shape, Fn&& fn) const {
        auto domain = _get_domain_early(sndr);
        return ::neutron::execution::transform_sender(
            domain, make_sender(
                        *this, _product_type<Shape, Fn>{ shape, fn },
                        std::forward<Sndr>(sndr)));
    }

    template <typename Policy, typename Shape, typename Fn>
    requires std::is_execution_policy_v<std::remove_cvref_t<Policy>>
    constexpr decltype(auto)
        operator()(const Policy& policy, Shape&& shape, Fn&& fn) const {
        return _sender_adaptor<
            const bulk_t, const Policy&, std::remove_cvref_t<Shape>, Fn>(
            *this, policy, shape, std::forward<Fn>(fn));
    }

    template <
        sender Sndr, typename Policy, std::integral Shape, movable_value Fn>
    requires std::is_execution_policy_v<std::remove_cvref_t<Policy>>
    constexpr decltype(auto) operator()(
        Sndr&& sndr, const Policy& policy, Shape&& shape, Fn&& fn) const {
        auto domain = _get_domain_early(sndr);
        return ::neutron::execution::transform_sender(
            domain,
            make_sender(
                *this,
                _product_type<const Policy&, Shape, Fn>{ policy, shape, fn },
                std::forward<Sndr>(sndr)));
    }
} bulk{};

namespace _bulk {

template <typename State>
struct _state_traits;

template <typename Shape, typename Fn>
struct _state_traits<_product_type<Shape, Fn>> {
    using shape_type = std::remove_cvref_t<Shape>;
    using fn_type    = Fn;

    static constexpr decltype(auto) shape(auto& state) noexcept {
        return std::get<0>(state);
    }

    static constexpr decltype(auto) fn(auto& state) noexcept {
        return std::get<1>(state);
    }
};

template <typename Policy, typename Shape, typename Fn>
struct _state_traits<_product_type<Policy, Shape, Fn>> {
    using shape_type = std::remove_cvref_t<Shape>;
    using fn_type    = Fn;

    static constexpr decltype(auto) shape(auto& state) noexcept {
        return std::get<1>(state);
    }

    static constexpr decltype(auto) fn(auto& state) noexcept {
        return std::get<2>(state);
    }
};

template <typename Shape, typename Fn, typename Cmplsigs>
struct _cmplsigs_for_impl_helper;

template <typename Shape, typename Fn, typename... Cmplsigs>
struct _cmplsigs_for_impl_helper<
    Shape, Fn, completion_signatures<Cmplsigs...>> {

    template <typename>
    struct _is_nothrow;

    template <typename Tag, typename... Args>
    struct _is_nothrow<Tag(Args...)> {
        static constexpr bool value =
            std::same_as<Tag, ::neutron::execution::set_value_t> &&
            std::is_nothrow_invocable_v<Fn, std::remove_cvref_t<Shape>, Args...>;
    };

    template <typename... Sigs>
    struct _is_nothrow<completion_signatures<Sigs...>> {
        static constexpr bool value =
            (false || ... || _is_nothrow<Sigs>::value);
    };

    using cmplsigs_t = completion_signatures<Cmplsigs...>;
    using type       = std::conditional_t<
        _is_nothrow<cmplsigs_t>::value, cmplsigs_t,
        completion_signatures<Cmplsigs..., set_error_t(std::exception_ptr)>>;
};

} // namespace _bulk

template <>
struct _impls_for<bulk_t> : _default_impls {
    static constexpr struct _complete_impl {
        template <typename Index, typename State, typename Rcvr, typename Tag, typename... Args>
        constexpr void operator()(
            Index, State& state, Rcvr& rcvr, Tag,
            Args&&... args) const noexcept {
            if constexpr (std::same_as<Tag, set_value_t>) {
                using traits  = _bulk::_state_traits<std::remove_cvref_t<State>>;
                using shape_t = typename traits::shape_type;
                auto&& shape  = traits::shape(state);
                auto&& fn     = traits::fn(state);

                constexpr bool nothrow =
                    noexcept(fn(shape_t{ shape }, args...));

                ATOM_TRY {
                    [shape, &fn, &rcvr, &args...]() noexcept(nothrow) {
                        for (shape_t i = 0; i < shape; ++i) {
                            fn(i, args...);
                        }
                        Tag()(std::move(rcvr), std::forward<Args>(args)...);
                    }();
                }
                ATOM_CATCH(...) {
                    if constexpr (!nothrow) {
                        ::neutron::execution::set_error(
                            std::move(rcvr), std::current_exception());
                    }
                }
            } else {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        }
    } complete{};
};
template <typename Data, typename Sndr, typename Env>
requires requires {
    typename _bulk::_state_traits<Data>::shape_type;
    typename _bulk::_state_traits<Data>::fn_type;
}
struct _completion_signatures_for_impl<_basic_sender<bulk_t, Data, Sndr>, Env> {
    using cmplsigs = decltype(get_completion_signatures(
        std::declval<Sndr>(), std::declval<Env>()));
    using traits   = _bulk::_state_traits<Data>;
    using type     = typename _bulk::_cmplsigs_for_impl_helper<
        typename traits::shape_type, typename traits::fn_type, cmplsigs>::type;
};

} // namespace neutron::execution
