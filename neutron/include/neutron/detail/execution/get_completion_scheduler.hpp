#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

template <typename Completion>
struct get_completion_scheduler_t {
    template <typename Env>
    auto operator()(Env env) const noexcept
    requires requires {
        { std::as_const(env).query(get_completion_scheduler_t{}) } noexcept;
    }
    {
        return std::as_const(env).query(*this);
    }
};

template <typename Completion>
inline constexpr get_completion_scheduler_t<Completion>
    get_completion_scheduler{};

} // namespace neutron::execution
