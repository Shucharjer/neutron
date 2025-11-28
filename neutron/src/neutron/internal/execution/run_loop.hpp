#pragma once
#include <condition_variable>
#include <exception>
#include <mutex>
#include <type_traits>
#include <utility>
#include "completion_signatures.hpp"
#include "queries.hpp"
#include "../macros.hpp"
#include "./fwd.hpp"

namespace neutron::execution {

namespace _run_loop {

class run_loop {
public:
    struct env;

    struct opstate_base {
        opstate_base* next = this;
        union {
            opstate_base* tail = nullptr;
            void (*_execute)(opstate_base*) noexcept;
        };

        void execute() noexcept { (*_execute)(this); }
    };

    template <typename Receiver>
    class opstate;
    class sender;
    class scheduler;

    run_loop() noexcept                  = default;
    run_loop(const run_loop&)            = delete;
    run_loop(run_loop&&)                 = delete;
    run_loop& operator=(const run_loop&) = delete;
    run_loop& operator=(run_loop&&)      = delete;
    ~run_loop();

    constexpr scheduler get_scheduler();
    void run();
    void finish();

private:
    opstate_base* _pop_front();
    void _push_back(opstate_base*);

    bool stop_ = false;
    std::mutex mutex_;
    std::condition_variable cv_;
    opstate_base head_{ &head_, { &head_ } };
};

struct run_loop::env {
    run_loop* loop;

    template <typename Completion>
    constexpr auto
        query(const get_completion_scheduler_t<Completion>&) const noexcept
        -> run_loop::scheduler;
};

template <typename Receiver>
class run_loop::opstate : run_loop::opstate_base {
public:
    using operation_state_concept = operation_state_t;

    template <typename Rcvr>
    constexpr opstate(run_loop* loop, Rcvr&& rcvr)
        : run_loop::opstate_base(this, &execute), loop_(loop),
          receiver_(std::forward<Rcvr>(rcvr)) {}

    constexpr void start() & noexcept {
        try {
            loop_->_push_back(this);
        } catch (...) {
            set_error(receiver_, std::current_exception());
        }
    }

    constexpr static void execute(opstate_base* base) noexcept {
        auto* const op = static_cast<opstate*>(base);
        if (get_stop_token(get_env(op->receiver_)).stop_requested()) {
            set_stopped(std::move(op->receiver_));
        } else {
            set_value(std::move(op->receiver_));
        }
    }

private:
    run_loop* loop_;
    Receiver receiver_;
};

class run_loop::sender {
public:
    using sender_concept        = sender_t;
    using completion_signatures = ::neutron::execution::completion_signatures<
        set_value_t(), set_error_t(), set_stopped_t()>;

    constexpr sender(run_loop* loop) noexcept : loop_(loop) {}

    NODISCARD constexpr auto get_env() const noexcept -> run_loop::env {
        return { this->loop_ };
    }

    template <typename Receiver>
    constexpr auto connect(Receiver&& receiver) noexcept
        -> run_loop::opstate<std::decay_t<Receiver>> {
        return { loop_, std::forward<Receiver>(receiver) };
    }

private:
    run_loop* loop_;
};

class run_loop::scheduler {
public:
    using scheduler_concept = scheduler_t;
    constexpr scheduler(run_loop* loop) noexcept : loop_(loop) {}
    constexpr auto schedule() -> run_loop::sender { return { this->loop_ }; }
    constexpr bool operator==(const scheduler&) const noexcept = default;

private:
    run_loop* loop_;
};

template <typename Completion>
constexpr auto run_loop::env::query(
    const get_completion_scheduler_t<Completion>&) const noexcept
    -> run_loop::scheduler {
    return run_loop::scheduler{ this->loop };
}

auto run_loop::_pop_front() -> run_loop::opstate_base* {
    std::unique_lock ulock{ mutex_ };
    cv_.wait(ulock, [this] { return head_.next != &head_ || stop_; });
    if (head_.next == head_.tail) {
        head_.tail = &head_;
    }
    return std::exchange(head_.next, head_.next->next);
}

void run_loop::_push_back(run_loop::opstate_base* op) {
    std::unique_lock ulock{ mutex_ };
    op->next   = &head_;
    head_.tail = head_.tail->next = op;
    cv_.notify_one();
}

constexpr run_loop::scheduler run_loop::get_scheduler() {
    return run_loop::scheduler{ this };
}

void run_loop::run() {
    for (run_loop::opstate_base* op{}; (op = _pop_front()) != &head_;) {
        op->execute();
    }
}

void run_loop::finish() {
    std::unique_lock ulock{ mutex_ };
    stop_ = true;
    cv_.notify_one();
}

} // namespace _run_loop

} // namespace neutron::execution
