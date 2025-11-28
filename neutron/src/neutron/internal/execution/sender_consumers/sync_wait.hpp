#pragma once
#include <exception>
#include "../fwd.hpp"

#include "../connect.hpp"
#include "../domain.hpp"
#include "../run_loop.hpp"
#include "../sender.hpp"
#include "../transform.hpp"

namespace neutron::execution {

constexpr inline struct sync_wait_t {

    struct env {
        _run_loop::run_loop* loop;

        NODISCARD constexpr auto query(get_scheduler_t) const noexcept {
            return loop->get_scheduler();
        }

        NODISCARD constexpr auto
            query(get_delegation_scheduler_t) const noexcept {
            return loop->get_scheduler();
        }
    };

    template <sender_in<sync_wait_t::env> Sndr>
    using result_t = value_types_of_t<Sndr, sync_wait_t::env>;

    template <typename Sndr>
    struct state {
        _run_loop::run_loop loop;
        std::exception_ptr error;
        result_t<Sndr> result;
    };

    template <typename Sndr>
    struct receiver {
        using receiver_concept = receiver_t;
        state<Sndr>* state;
    };

    template <sender Sndr>
    constexpr decltype(auto) apply_sender(Sndr&& sndr) const {
        sync_wait_t::state<Sndr> state;
        auto op = connect(
            std::forward<Sndr>(sndr), sync_wait_t::receiver<Sndr>{ &state });
        start(op);
        state.loop.run();
        if (state.error) {
            std::rethrow_exception(state.error);
        }
        return std::move(state.result);
    }

    template <sender_in<sync_wait_t::env> Sndr>
    constexpr decltype(auto) operator()(Sndr&& sndr) const {
        auto domain = _get_early_domain(sndr);
        return ::neutron::execution::apply_sender(
            domain, *this, std::forward<Sndr>(sndr));
    };
} sync_wait;

} // namespace neutron::execution
