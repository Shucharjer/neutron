#pragma once
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_delegation_scheduler.hpp"
#include "neutron/detail/execution/get_scheduler.hpp"
#include "neutron/detail/execution/run_loop.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/utility/as_except_ptr.hpp"

namespace neutron::this_thread {

struct _sync_wait_env {
    execution::run_loop* loop;
    auto query(execution::get_scheduler_t) const noexcept {
        return loop->get_scheduler();
    }

    auto query(execution::get_delegation_scheduler_t) const noexcept {
        return loop->get_scheduler();
    }
};

template <execution::sender_in<_sync_wait_env> Sndr>
using _sync_wait_result_type = std::optional<execution::value_types_of_t<
    Sndr, _sync_wait_env, execution::_decayed_tuple, std::type_identity_t>>;

template <execution::sender_in<_sync_wait_env> Sndr>
using _sync_wait_with_variant_result_type =
    std::optional<execution::value_types_of_t<Sndr, _sync_wait_env>>;

template <typename Sndr>
struct _sync_wait_state { // exposition only
    execution::run_loop loop;
    std::exception_ptr error;
    _sync_wait_result_type<Sndr> result;
};

template <typename Sndr>
struct _sync_wait_receiver {
    using receiver_concept = execution::receiver_t;

    _sync_wait_state<Sndr>* state;

    template <typename... Args>
    void set_value(Args&&... args) && noexcept {
        ATOM_TRY { state->result.emplace(std::forward<Args>(args)...); }
        ATOM_CATCH(...) { state->error = std::current_exception(); }
        state->loop.finish();
    }

    template <typename Error>
    void set_error(Error&& err) && noexcept {
        state->error = as_except_ptr(std::forward<Error>(err));
        state->loop.finish();
    }

    void set_stopped() && noexcept { state->loop.finish(); }

    _sync_wait_env get_env() const noexcept { return { &state->loop }; }
};

struct sync_wait_t {
    template <typename Sndr>
    auto apply_sender(Sndr&& sndr) const {
        _sync_wait_state<Sndr> state;
        auto op = execution::connect(
            std::forward<Sndr>(sndr), _sync_wait_receiver<Sndr>{ &state });
        start(op);

        state.loop.run();
        if (state.error) {
            std::rethrow_exception(std::move(state.error));
        }
        return std::move(state.result);
    }

    template <execution::sender Sndr>
    void operator()(Sndr&& sndr) const {
        auto dom = _get_domain_early(sndr);
        return execution::apply_sender(dom, *this, std::forward<Sndr>(sndr));
    }
};

inline constexpr sync_wait_t sync_wait;

} // namespace neutron::this_thread
