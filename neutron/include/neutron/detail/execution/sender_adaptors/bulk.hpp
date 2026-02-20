#pragma once
#include <exception>
#include "neutron/detail/concepts/movable_value.hpp"
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/product_type.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"
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
                    [shape, &rcvr, &args...]() noexcept(nothrow) {
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

template <typename Shape, typename Fn, typename Sndr, typename Env>
struct _completion_signatures_for_impl<_basic_sender<bulk_t, _product_type<Shape, Fn>, Sndr>, Env> {
    using type = /* TODO */;
};

} // namespace neutron::execution
