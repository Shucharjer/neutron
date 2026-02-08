#pragma once
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

inline constexpr struct schedule_t {
    template <typename Sch>
    constexpr decltype(auto) operator()(Sch&& sch) const {
        return std::forward<Sch>(sch).schedule();
    }
} schedule;

} // namespace neutron::execution
