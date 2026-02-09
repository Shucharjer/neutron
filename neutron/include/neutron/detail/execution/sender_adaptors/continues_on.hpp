#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/sched_attrs.hpp"
#include "neutron/detail/execution/sender_adaptor.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron::execution {

inline constexpr struct continues_on_t {
    template <sender_for<continues_on_t> Sndr, typename... Env>
    constexpr static auto transform_sender(Sndr&& sndr, Env&&... env) {
        auto&& data  = get<1>(sndr);
        auto&& child = get<2>(sndr);
        return schedule_from(std::move(data), std::move(child));
    }

    template <scheduler Scheduler>
    constexpr auto operator()(Scheduler&& scheduler) const {
        return _sender_adaptor(*this, std::forward<Scheduler>(scheduler));
    }

    template <sender Sndr, scheduler Scheduler>
    constexpr sender auto operator()(Sndr&& sndr, Scheduler&& scheduler) const {
        auto domain = _get_domain_early(sndr);
        return ::neutron::execution::transform_sender(
            domain, std::forward<Sndr>(sndr),
            std::forward<Scheduler>(scheduler));
    }
} continues_on;

template <>
struct _impls_for<continues_on_t> : _default_impls {
    static constexpr auto get_attrs =
        [](const auto& data, const auto& child) noexcept -> decltype(auto) {
        return _join_env(_sched_attrs(data), _fwd_env(get_env(child)));
    };
};

} // namespace neutron::execution
