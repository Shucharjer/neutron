#pragma once
#include "neutron/detail/concepts/movable_value.hpp"
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"

namespace neutron::execution {

struct let_value_t {
    template <movable_value Fn>
    constexpr auto operator()(Fn&& fn) const {
        return _sender_adaptor(*this, std::forward<Fn>(fn));
    }

    template <sender Sndr, movable_value Fn>
    constexpr auto operator()(Sndr&& sndr, Fn&& fn) const {
        auto domain = _get_domain_early(sndr);
        return ::neutron::execution::transform_sender(
            domain,
            make_sender(*this, std::forward<Fn>(fn), std::forward<Sndr>(sndr)));
    }
};

inline constexpr let_value_t let_value;

template <>
struct _impls_for<let_value_t> : _default_impls {
    //
};

} // namespace neutron::execution
