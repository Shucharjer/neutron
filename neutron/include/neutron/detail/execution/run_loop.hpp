#pragma once
#include <condition_variable>
#include <exception>
#include <mutex>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/unstoppable_token.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_completion_scheduler.hpp"
#include "neutron/detail/execution/set_error.hpp"
#include "neutron/detail/execution/set_stopped.hpp"
#include "neutron/detail/execution/set_value.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

class run_loop {
    struct _run_loop_opstate_base {
        _run_loop_opstate_base* next;

        union {
            _run_loop_opstate_base* tail;
            void (*execute_fn)(_run_loop_opstate_base*) noexcept;
        };

        constexpr void execute() noexcept { (*execute_fn)(this); }

        constexpr _run_loop_opstate_base() noexcept : next(this), tail(this) {}

        constexpr _run_loop_opstate_base(
            _run_loop_opstate_base* next,
            void (*fn)(_run_loop_opstate_base*) noexcept) noexcept
            : next(next), execute_fn(fn) {}
    };

    template <typename Rcvr>
    struct _run_loop_opstate : _run_loop_opstate_base {
        using operation_state_concept = operation_state_t;
        run_loop* loop;
        ATOM_NO_UNIQUE_ADDR Rcvr rcvr;

        template <typename R>
        _run_loop_opstate(run_loop* loop, R&& rcvr) noexcept(
            std::is_nothrow_constructible_v<Rcvr, R>)
            : _run_loop_opstate_base(this, &execute), loop(loop),
              rcvr(std::forward<Rcvr>(rcvr)) {}

        static constexpr void execute(_run_loop_opstate_base* task) noexcept {
            auto* const self = static_cast<_run_loop_opstate*>(task);
            using token      = decltype(get_stop_token(get_env(self->rcvr)));
            ATOM_TRY {
                if constexpr (not unstoppable_token<token>) {
                    if (get_stop_token(get_env(self->rcvr)).stop_requested()) {
                        set_stopped(std::move(self->rcvr));
                    } else {
                        set_value(std::move(self->rcvr));
                    }
                } else {
                    set_value(std::move(self->rcvr));
                }
            }
            ATOM_CATCH(...) {
                set_error(std::move(self->rcvr), std::current_exception());
            }
        }

        constexpr void start() {
            ATOM_TRY { loop->_push_back(this); }
            ATOM_CATCH(...) {
                set_error(static_cast<Rcvr&&>(rcvr), std::current_exception());
            }
        }
    };

    class _run_loop_scheduler;

    struct _run_loop_env {
        run_loop* loop;

        template <typename Completion>
        auto query(const get_completion_scheduler_t<Completion>&) const noexcept
            -> _run_loop_scheduler {
            return { this->loop };
        }
    };

    struct _run_loop_sender {
        using sender_concept = sender_t;
        using completion_signatures =
            ::neutron::execution::completion_signatures<
                set_value_t(), set_error_t(std::exception_ptr),
                set_stopped_t()>;

        auto get_env() const noexcept -> _run_loop_env { return { loop }; }

        template <typename Rcvr>
        auto connect(Rcvr&& rcvr) noexcept
            -> _run_loop_opstate<std::decay_t<Rcvr>> {
            return { loop, std::forward<Rcvr>(rcvr) };
        }

        run_loop* loop;
    };

    struct _run_loop_scheduler {
        using scheduler_concept = scheduler_t;
        auto schedule() const noexcept -> _run_loop_sender {
            return { this->loop };
        }
        bool operator==(const _run_loop_scheduler& that) const = default;
        run_loop* loop;
    };

    _run_loop_opstate_base* _pop_front() {
        std::unique_lock guard{ mutex_ };
        cv_.wait(guard, [this] { return head_.next != &head_ || stop_; });
        if (head_.tail == head_.next) {
            head_.tail = &head_;
        }
        return std::exchange(head_.next, head_.next->next);
    }

    void _push_back(_run_loop_opstate_base* task) {
        std::unique_lock guard{ mutex_ };
        task->next = &head_;
        head_.tail = head_.tail->next = task;
        cv_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    _run_loop_opstate_base head_;
    bool stop_ = false;

public:
    run_loop() noexcept  = default;
    run_loop(run_loop&&) = delete;

    _run_loop_scheduler get_scheduler() { return _run_loop_scheduler{ this }; }

    void run() {
        for (_run_loop_opstate_base* opstate{};
             (opstate = _pop_front()) != &head_;) {
            opstate->execute();
        }
    }

    void finish() {
        std::unique_lock guard{ mutex_ };
        stop_ = true;
        cv_.notify_one();
    }
};

} // namespace neutron::execution
