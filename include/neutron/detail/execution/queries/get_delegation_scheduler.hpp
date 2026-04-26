#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

struct get_delegation_scheduler_t {
    template <typename Env>
    constexpr scheduler auto operator()(Env&& env) noexcept {
        static_assert(noexcept(std::as_const(env).query(*this)));
        return std::as_const(env).query(*this);
    }
};

inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

} // namespace neutron::execution
