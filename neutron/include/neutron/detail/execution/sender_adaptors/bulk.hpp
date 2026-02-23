#pragma once
#include <concepts>
#include <exception>
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
    constexpr decltype(auto) operator()(Shape&& shape, Fn&& fn) const {
        return _sender_adaptor(*this, shape, std::forward<Fn>(fn));
    }

    template <sender Sndr, std::integral Shape, movable_value Fn>
    constexpr decltype(auto)
        operator()(Sndr&& sndr, Shape&& shape, Fn&& fn) const {
        auto domain = _get_domain_early(sndr);
        return ::neutron::execution::transform_sender(
            domain, make_sender(
                        *this, _product_type<Shape, Fn>{ shape, fn },
                        std::forward<Sndr>(sndr)));
    }
} bulk{};

template <>
struct _impls_for<bulk_t> : _default_impls {
    static constexpr struct _complete_impl {
        template <
            typename Index, typename Shape, typename Fn, typename Rcvr,
            typename Tag, typename... Args>
        constexpr void operator()(
            Index, _product_type<Shape, Fn>& state, Rcvr& rcvr, Tag,
            Args&&... args) const noexcept {
            if constexpr (std::same_as<Tag, set_value_t>) {
                auto& [shape, fn] = state;
                using shape_t     = std::remove_cvref_t<decltype(shape)>;

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

namespace _bulk {

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
            std::is_nothrow_invocable_v<Fn, Shape, Args...>;
    };

    template <typename... Sigs>
    struct _is_nothrow<completion_signatures<Sigs...>> {
        static constexpr bool value =
            (false || ... || _is_nothrow<Sigs>::value);
    };

    using type = std::conditional_t<
        _is_nothrow<Cmplsigs...>::value, completion_signatures<Cmplsigs...>,
        completion_signatures<Cmplsigs..., set_error_t(std::exception_ptr)>>;
};

} // namespace _bulk

template <typename Shape, typename Fn, typename Sndr, typename Env>
struct _completion_signatures_for_impl<
    _basic_sender<bulk_t, _product_type<Shape, Fn>, Sndr>, Env> {
    using cmplsigs = decltype(get_completion_signatures(
        std::declval<Sndr>(), std::declval<Env>()));
    using type =
        typename _bulk::_cmplsigs_for_impl_helper<Shape, Fn, cmplsigs>::type;
};

} // namespace neutron::execution
