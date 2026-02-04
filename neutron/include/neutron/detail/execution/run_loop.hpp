#pragma once
#include <condition_variable>
#include <exception>
#include <mutex>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/unstoppable_token.hpp"
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

class run_loop {
    class _run_loop_scheduler {
    public:
        constexpr _run_loop_scheduler(run_loop* loop);
    };
    class _run_loop_sender;

    struct _run_loop_opstate_base {
        using _execute_fn_t = void(_run_loop_opstate_base*) noexcept;
        constexpr _run_loop_opstate_base() = default;

        constexpr explicit _run_loop_opstate_base(_execute_fn_t* fn) noexcept
            : execute_fn(fn) {}

        constexpr void execute() noexcept { (*execute_fn)(this); }

        _execute_fn_t* execute_fn{};
        _run_loop_opstate_base* next{};
    };

    template <typename Rcvr>
    struct _run_loop_opstate : _run_loop_opstate_base {
        using operation_state_concept = operation_state_t;
        run_loop* loop;
        Rcvr rcvr;

        template <typename R>
        _run_loop_opstate(run_loop* loop, R&& rcvr) noexcept(
            std::is_nothrow_constructible_v<Rcvr, R>)
            : loop(loop), rcvr(std::forward<Rcvr>(rcvr)) {}

        static constexpr void execute(_run_loop_opstate_base* task) noexcept {
            auto* const self = static_cast<_run_loop_opstate*>(task);
            using token      = decltype(get_stop_token(get_env(self->rcvr)));
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

        constexpr void start() {}
    };

    enum class _state {
        running,
        finishing
    };

    _run_loop_opstate_base* _pop_front() {
        std::unique_lock guard{ mutex_ };
        cv_.wait(guard, [this] { return front_; });
        if (front_ == back_) {
            back_ = nullptr;
        }
        return front_ != nullptr ? std::exchange(front_, front_->next)
                                 : nullptr;
    }

    void _push_back(_run_loop_opstate_base* task) {
        std::lock_guard guard{ mutex_ };
        auto* pback = std::exchange(back_, task);
        if (pback != nullptr) {
            pback->next = task;
        } else {
            front_ = task;
            cv_.notify_one();
        }
    }

    _state state_;
    std::mutex mutex_;
    std::condition_variable cv_;
    _run_loop_opstate_base* front_;
    _run_loop_opstate_base* back_;

public:
    run_loop() noexcept;
    run_loop(run_loop&&) = delete;
    ~run_loop();

    _run_loop_scheduler get_scheduler() { return _run_loop_scheduler{ this }; }

    void run() {
        if (std::lock_guard guard{ mutex_ };
            state_ != _state::finishing &&
            std::exchange(state_, _state::running) == _state::running) {
            std::terminate();
        }

        _run_loop_opstate_base* op{};
        while ((op = { this->_pop_front() }) != nullptr) {
            op->execute();
        }
    }

    void finish() {
        {
            std::lock_guard guard { mutex_ };
            state_ = _state::finishing;
        }

        cv_.notify_one();
    }
};

} // namespace neutron::execution
