#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/queries.hpp"

namespace neutron::execution {

struct get_scheduler_t {
    template <typename Env>
    scheduler auto operator()(Env&& env) const noexcept
    requires (noexcept(std::as_const(env).query(*this))){
        return std::as_const(env).query(*this);
    }

    constexpr bool query(forwarding_query_t) const noexcept {
        return true;
    }
};

inline constexpr get_scheduler_t get_scheduler{};

}