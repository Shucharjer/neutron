#pragma once
// NOTE: tag_invoke lives under concepts/, not top-level
#include "../concepts/tag_invoke.hpp"
#include "./fwd.hpp"

namespace neutron::execution {

constexpr inline struct start_t {

    template <typename State>
    constexpr static bool has_member_start =
        requires(std::remove_cvref_t<State> state) {
            { state.start() } noexcept;
        };

    template <typename State>
    requires has_member_start<State> || tag_invocable<start_t, State>
    void operator()(const State& state) const noexcept {
        if constexpr (has_member_start<State>) {
            return state.start();
        } else {
            return tag_invoke(start_t{}, state);
        }
    }

    template <typename State>
    requires has_member_start<State> || tag_invocable<start_t, State>
    void operator()(State& state) const noexcept {
        if constexpr (has_member_start<State>) {
            return state.start();
        } else {
            return tag_invoke(start_t{}, state);
        }
    }
} start;

} // namespace neutron::execution
