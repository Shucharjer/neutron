#pragma once
#include <concepts>
#include <utility>
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

struct forwarding_query_t {
    template <typename Query>
    constexpr bool operator()(Query&& object) const noexcept {
        if constexpr (requires { object.query(*this); }) {
            return true;
        } else {
            return std::derived_from<Query, forwarding_query_t>;
        }
    }
};

inline constexpr forwarding_query_t forwarding_query{};

struct get_allocator_t {
    template <typename Query>
    constexpr bool operator()(Query&& object) const noexcept
    requires requires(const Query& object) {
        { std::as_const(object).query(*this) } noexcept -> std_simple_allocator;
    }
    {
        return std::as_const(object).query(*this);
    }

    constexpr bool query(forwarding_query_t) const noexcept { return true; }
};

inline constexpr get_allocator_t get_allocator{};

struct get_stop_token_t {
    template <typename Query>
    constexpr auto operator()(Query&& object) const noexcept
    requires requires {
        { std::as_const(object).query(*this) } noexcept;
    }
    {
        return std::as_const(object).query(*this);
    }

    template <typename Query>
    constexpr auto operator()(Query&& object) const noexcept
    requires(!requires {
        { std::as_const(object).query(*this) } noexcept;
    })
    {
        return ::neutron::never_stop_token{};
    }

    constexpr bool query(forwarding_query_t) const noexcept { return true; }
};

inline constexpr get_stop_token_t get_stop_token{};

} // namespace neutron::execution
