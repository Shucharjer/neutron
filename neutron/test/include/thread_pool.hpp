#pragma once
#define ATOM_EXECUTION
#ifndef ATOM_EXECUTION
    #error
#endif

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/concepts.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/execution.hpp"

namespace thread_pool_for_test {

using namespace neutron::execution;

class thread_pool;

class _domain;
class _env;
class _scheduler;
class _sender;

struct _task_base {
    _task_base* next                      = nullptr;
    void (*execute)(_task_base*) noexcept = nullptr;
};

class _domain : default_domain {
public:
    using default_domain::transform_env;
    using default_domain::apply_sender;

    template <typename Sndr, typename... Env>
    constexpr sender auto
        transform_sender(Sndr&& sndr, const Env&... env) noexcept {
        return default_domain::transform_sender(
            std::forward<Sndr>(sndr), env...);
    }
};

class _env {
public:
    explicit _env(thread_pool* pool) noexcept : pool_(pool) {}

    template <typename CPO>
    auto query(get_completion_scheduler_t<CPO>) const noexcept;

private:
    thread_pool* pool_;
};

class _scheduler {
public:
    using scheduler_concept = ::neutron::execution::scheduler_t;

    explicit _scheduler(thread_pool* pool) noexcept : pool_(pool) {}

    _sender schedule();

    bool operator==(const _scheduler& that) const noexcept {
        return pool_ == that.pool_;
    }

private:
    thread_pool* pool_;
};

template <typename Rcvr>
class _opstate : public _task_base {
public:
    using operation_state_concept = operation_state_t;

    explicit _opstate(thread_pool* pool, Rcvr rcvr)
        : pool_(pool), rcvr_(std::move(rcvr)) {
        // NOLINTNEXTLINE
        this->execute = [](_task_base* base) noexcept {
            auto& self  = *static_cast<_opstate*>(base);
            auto stoken = get_stop_token(get_env(self.rcvr_));

            if constexpr (neutron::unstoppable_token<decltype(stoken)>) {
                set_value(std::move(self.rcvr_));
            } else if (stoken.stop_requested()) {
                set_stopped(std::move(self.rcvr_));
            } else {
                set_value(std::move(self.rcvr_));
            }
        };
    }

    void start() & noexcept {
        ATOM_TRY { _enqueue(this); }
        ATOM_CATCH(...) {
            set_error(std::move(rcvr_), std::current_exception());
        }
    }

private:
    void _enqueue(_task_base* task) const;

    thread_pool* pool_;
    ATOM_NO_UNIQUE_ADDR Rcvr rcvr_;
};

class _sender {
public:
    using sender_concept        = neutron::execution::sender_t;
    using completion_signatures = neutron::execution::completion_signatures<
        set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;

    explicit _sender(thread_pool* pool) noexcept : pool_(pool) {}

    auto get_env() const noexcept { return _env{ pool_ }; }

    template <receiver Rcvr>
    auto connect(Rcvr&& rcvr) const
        noexcept(std::is_nothrow_constructible_v<std::decay_t<Rcvr>, Rcvr>)
            -> _opstate<std::decay_t<Rcvr>> {
        return _opstate<std::decay_t<Rcvr>>{ pool_, std::forward<Rcvr>(rcvr) };
    }

private:
    thread_pool* pool_;
};

inline _sender _scheduler::schedule() { return _sender{ pool_ }; }

class thread_pool {
    template <typename>
    friend class _opstate;

public:
    _scheduler get_scheduler() { return _scheduler{ this }; }

    explicit thread_pool(size_t n = std::thread::hardware_concurrency()) {
        if (n == 0) [[unlikely]] {
            n = 1;
        }

        for (size_t i = 0; i < n; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    _task_base* task = nullptr;
                    {
                        std::unique_lock slock{ mutex_ };
                        cv_.wait(
                            slock, [this] { return stop_ || !tasks_.empty(); });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = tasks_.front();
                        tasks_.pop();
                    }

                    ATOM_TRY { task->execute(task); }
                    ATOM_CATCH(...) {}
                }
            });
        }
    }

    ~thread_pool() {
        {
            std::lock_guard lock{ mutex_ };
            stop_ = true;
        }

        cv_.notify_all();
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    void _enqueue(_task_base* task) {
        {
            std::lock_guard lock{ mutex_ };
            if (stop_) {
                throw std::runtime_error("thread_pool is stopping");
            }

            tasks_.push(task);
        }

        cv_.notify_one();
    }

    std::vector<std::thread> threads_;
    std::queue<_task_base*> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

template <typename CPO>
inline auto _env::query(get_completion_scheduler_t<CPO>) const noexcept {
    return pool_->get_scheduler();
}

template <typename Rcvr>
void _opstate<Rcvr>::_enqueue(_task_base* task) const {
    pool_->_enqueue(task);
}

namespace _asserts {

static_assert(scheduler<_scheduler>);
static_assert(sender<_sender>);

} // namespace _asserts

} // namespace thread_pool_for_test

using thread_pool = thread_pool_for_test::thread_pool;
