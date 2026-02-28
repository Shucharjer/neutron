#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

struct get_await_completion_adaptor_t {
    template <typename Env>
    constexpr auto operator()(Env&& env) const noexcept
    requires requires {
        { std::as_const(env).query(*this) } noexcept;
    }
    {
        std::as_const(env).query(*this);
    }

    ATOM_NODISCARD constexpr bool
        forwarding_query(get_await_completion_adaptor_t) const noexcept {
        return true;
    }
};

inline constexpr get_await_completion_adaptor_t get_await_completion_adaptor{};

} // namespace neutron::execution
