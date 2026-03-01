#pragma once
#include <concepts>
#include <memory>
#include <type_traits>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

class task_scheduler {
    class _ts_sender;

    template <receiver Rcvr>
    class _state;

    friend struct _sched_helper;

public:
    using scheduler_concept = scheduler_t;

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

private:
    std::shared_ptr<void> sch_; // exposition only
};

class task_scheduler::_ts_sender {
public:
    using sender_concept = sender_t;

    template <receiver Rcvr>
    task_scheduler::_state<Rcvr> connect(Rcvr&& rcvr) &&;

private:
};

template <receiver R>
class task_scheduler::_state { // exposition only
public:
    using operation_state_concept = operation_state_t;

    void start() & noexcept;
};

struct _sched_helper {
    static auto& get(const task_scheduler& s) { return s.sch_; }
};
inline auto _sched(const task_scheduler& s) noexcept {
    return _sched_helper::get(s);
}

} // namespace neutron::execution
