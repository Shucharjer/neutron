#pragma once
#include <concepts>
#include <memory>
#include <type_traits>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/execution/parallel_scheduler_replacement/parallel_scheduler_backend.hpp"
#include "neutron/detail/execution/queries/get_completion_scheduler.hpp"
#include "neutron/detail/execution/queries/get_forward_progress_guarantee.hpp"
#include "neutron/detail/execution/query_with_default.hpp"
#include "neutron/detail/execution/receivers/set_value.hpp"
#include "neutron/detail/execution/sched_attrs.hpp"
#include "neutron/detail/execution/senders/default_domain.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/stop_token/unstoppable_token.hpp"

namespace neutron::execution {

template <typename Sch, typename Env>
concept _infallible_scheduler =
    scheduler<Sch> &&
    (std::same_as<
         completion_signatures<set_value_t()>,
         completion_signatures_of_t<decltype(schedule(declval<Sch>())), Env>> ||
     (!unstoppable_token<stop_token_of_t<Env>> &&
      (std::same_as<
           completion_signatures<set_value_t(), set_stopped_t()>,
           completion_signatures_of_t<
               decltype(schedule(declval<Sch>())), Env>> ||
       std::same_as<
           completion_signatures<set_stopped_t(), set_value_t()>,
           completion_signatures_of_t<
               decltype(schedule(declval<Sch>())), Env>>)));

class task_scheduler {
    class _ts_domain;

    template <scheduler Sch>
    class _backend_for;

    class _ts_sender;

    template <receiver Rcvr>
    class _state;

    friend struct _sched_helper;

public:
    using scheduler_concept = scheduler_tag;

    template <class Sch, class Allocator = std::allocator<void>>
    requires(!std::same_as<task_scheduler, std::remove_cvref_t<Sch>>) &&
            scheduler<Sch>
    explicit task_scheduler(Sch&& sch, Allocator alloc = {});

    task_scheduler(const task_scheduler&)            = default;
    task_scheduler& operator=(const task_scheduler&) = default;

    _ts_sender schedule();

    friend bool operator==(
        const task_scheduler& lhs, const task_scheduler& rhs) noexcept {
        return lhs.sch_ == rhs.sch_;
    }

    // template <class Sch>
    // requires(!std::same_as<task_scheduler, Sch>) && scheduler<Sch>
    // friend bool operator==(const task_scheduler& lhs, const Sch& rhs)
    // noexcept;

    ATOM_NODISCARD constexpr auto
        query(get_forward_progress_guarantee_t qry) const noexcept
        -> forward_progress_guarantee {
        // return sch_->query(qry);
        return {};
    }

    ATOM_NODISCARD constexpr auto
        query(get_completion_scheduler_t<set_value_t>) const noexcept
        -> const task_scheduler& {
        return *this;
    }

private:
    std::shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend>
        sch_;
    // std::shared_ptr<void> sch_; // exposition only
};

template <scheduler Sch>
class task_scheduler::_backend_for :
    public parallel_scheduler_replacement::parallel_scheduler_backend {
public:
    explicit _backend_for(Sch sch) : sched_(std::move(sch)) {}

    void schedule(
        parallel_scheduler_replacement::receiver_proxy& rcvr,
        std::span<std::byte> span) noexcept override {
        // auto os = connect(schedule(sched_), _WRAP_RCVR(rcvr));
        // start(os);
    }
    void schedule_bulk_chunked(
        size_t shape,
        parallel_scheduler_replacement::bulk_item_receiver_proxy& rcvr,
        std::span<std::byte> span) noexcept override;
    void schedule_bulk_unchunked(
        size_t shape,
        parallel_scheduler_replacement::bulk_item_receiver_proxy& rcvr,
        std::span<std::byte> span) noexcept override;

private:
    Sch sched_;
};

class task_scheduler::_ts_sender {
public:
    using sender_concept = sender_tag;

    template <receiver Rcvr>
    task_scheduler::_state<Rcvr> connect(Rcvr&& rcvr) &&;

    ATOM_NODISCARD
    constexpr auto get_env() const noexcept
        -> const _sched_attrs<task_scheduler>& {
        return attrs_;
    }

private:
    _sched_attrs<task_scheduler> attrs_;
};

constexpr auto _not_a_scheduler() noexcept {
    return "the expr is not a scheduler";
}

class task_scheduler::_ts_domain : public default_domain {
public:
    template <typename BulkSndr, typename Env>
    requires sender_in<BulkSndr, Env> &&
             (sender_for<BulkSndr, bulk_chunked_t> ||
              sender_for<BulkSndr, bulk_unchunked_t>)
    static constexpr auto transform_sender(
        set_value_t, BulkSndr&& bulk_sndr,
        const Env& env) noexcept(std::
                                     is_nothrow_constructible_v<
                                         std::decay_t<BulkSndr>, BulkSndr>) {
        auto& [_, data, child] = bulk_sndr;
        auto& [_, shape, fn]   = data;
        auto sch               = _call_with_default(
            get_completion_scheduler<set_value_t>, _not_a_scheduler(),
            get_env(child), _fwd_env(env));
        using sch_t = std::remove_cvref_t<decltype(sch)>;
        if constexpr (std::same_as<sch_t, task_scheduler>) {
            // ...
        } else {
            return _not_a_scheduler();
        }
    }
};

template <receiver R>
class task_scheduler::_state { // exposition only
public:
    using operation_state_concept = operation_state_tag;

    void start() & noexcept {
        //
    }
};

struct _sched_helper {
    static auto& get(const task_scheduler& s) { return s.sch_; }
};
inline auto _sched(const task_scheduler& s) noexcept {
    return _sched_helper::get(s);
}

} // namespace neutron::execution
