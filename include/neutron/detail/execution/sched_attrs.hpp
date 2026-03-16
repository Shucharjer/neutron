#pragma once
#include <concepts>
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"

namespace neutron::execution {

template <typename Scheduler>
class _sched_attrs {
private:
    Scheduler sched_;

public:
    template <typename S>
    requires(!::std::same_as<_sched_attrs, ::std::remove_cvref_t<S>>)
    explicit _sched_attrs(S&& sched) : sched_(::std::forward<S>(sched)) {}

    template <typename Tag>
    auto query(const get_completion_scheduler_t<Tag>&) const noexcept {
        return this->sched_;
    }

    template <typename T = bool>
    requires requires(Scheduler&& sched) { sched.query(get_domain); }
    auto query(const get_domain_t& q, T = true) const noexcept {
        return this->sched_.query(q);
    }
};

template <typename Scheduler>
_sched_attrs(Scheduler&&) -> _sched_attrs<::std::remove_cvref_t<Scheduler>>;

} // namespace neutron::execution
