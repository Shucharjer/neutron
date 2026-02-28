#pragma once
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/join_env.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/sched_attrs.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"

namespace neutron::execution {

struct affine_on_t {
    constexpr auto operator()() const { return _sender_adaptor(*this); }

    template <sender Sndr>
    constexpr auto operator()(Sndr&& sndr) const {
        auto domain = _get_domain_early(sndr);
        return transform_sender(
            domain, make_sender(*this, empty_env{}, std::forward<Sndr>(sndr)));
    }

    template <sender Sndr, typename Env>
    constexpr auto transform_sender(Sndr&& sndr, const Env& env) {
        // TODO
    }
};

inline constexpr affine_on_t affine_on;

template <>
struct _impls_for<affine_on_t> : _default_impls {
    struct _get_attrs_impl {
        constexpr auto operator()(const auto& data, const auto& child) {
            return _join_env(_sched_attrs(data), _fwd_env(get_env(child)));
        }
    };
    static constexpr _get_attrs_impl get_attrs{};
};

} // namespace neutron::execution
