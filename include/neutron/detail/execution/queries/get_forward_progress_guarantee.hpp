#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

inline constexpr struct get_forward_progress_guarantee_t {
    template <scheduler Sch>
    constexpr auto operator()(Sch&& scheduler) const noexcept
    requires requires {
        { std::as_const(scheduler).query(*this) } noexcept;
    }
    {
        return std::as_const(scheduler).query(*this);
    }

    template <scheduler Sch>
    constexpr auto operator()(Sch&& scheduler) const noexcept
    requires(!requires {
        { std::as_const(scheduler).query(*this) } noexcept;
    })
    {
        static_assert(
            false, "the query of forward_progress_guarantee should be nothrow");
    }
} get_forward_progress_guarantee;

} // namespace neutron::execution
