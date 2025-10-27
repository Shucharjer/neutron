#pragma once
#include <atomic>
#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace neutron {

template <typename Ty>
class coroutine {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        coroutine get_return_object() { return coroutine(handle_type::from_promise(*this)); }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        template <typename T>
        std::suspend_always yield_value(T&& val) {
            value_ = std::forward<T>(val);
            return {};
        }
        template <typename T>
        void return_value(T&& val) {
            value_ = std::forward<T>(val);
        }
        void unhandled_exception() { eptr_ = std::current_exception(); }

        Ty get() {
            if (eptr_) [[unlikely]] {
                std::rethrow_exception(eptr_);
            }

            return value_;
        }

    private:
        Ty value_;
        std::exception_ptr eptr_;
    };

    coroutine(std::coroutine_handle<promise_type> coro) : handle_(coro) {}

    coroutine(const coroutine& that) : handle_(that.handle_) {}

    coroutine(coroutine&& that) noexcept : handle_(std::exchange(that.handle_, nullptr)) {}

    coroutine& operator=(const coroutine& that) { handle_ = that.handle_; }

    coroutine& operator=(coroutine&& that) noexcept {
        handle_ = std::exchange(that.handle_, nullptr);
    }

    ~coroutine() {
        if (handle_) {
            handle_.destroy();
        }
    }

    operator bool() const noexcept { return !handle_.done(); }

    Ty get() const {
        if (!handle_ || handle_.done()) [[unlikely]] {
            throw std::runtime_error("Invalid coroutine state.");
        }
        handle_.resume();
        return handle_.promise().get();
    }

private:
    handle_type handle_;
};

template <typename Ty>
class thread_safe_coroutine {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    enum class status : uint8_t {
        suspended,
        running,
        completed
    };

    struct promise_type {
        std::atomic<status> state{ status::suspended };
        std::mutex mtx;
        std::condition_variable cv;
        Ty value;
        std::exception_ptr eptr;

        thread_safe_coroutine get_return_object() {
            return thread_safe_coroutine(
                handle_type::from_promise(*this), std::make_shared<control_block>(this));
        }

        std::suspend_always initial_suspend() { return {}; }

        auto final_suspend() noexcept {
            struct final_awaiter : std::suspend_always {
                promise_type& promise;
                explicit final_awaiter(promise_type& primise) : promise(primise) {}

                bool await_ready() noexcept {
                    std::unique_lock lk(promise.mtx);
                    promise.state.store(status::completed);
                    promise.cv.notify_all();
                    return false;
                }
            };
            return final_awaiter{ *this };
        }

        template <typename T>
        std::suspend_always yield_value(T&& val) {
            std::unique_lock lk(mtx);
            value = std::forward<T>(val);
            state.store(status::suspended);
            cv.notify_one();
            return {};
        }

        template <typename T>
        void return_value(T&& val) {
            std::unique_lock lk(mtx);
            value = std::forward<T>(val);
            state.store(status::completed);
            cv.notify_all();
        }

        void unhandled_exception() {
            std::unique_lock lk(mtx);
            eptr = std::current_exception();
            state.store(status::completed);
            cv.notify_all();
        }

        Ty get() {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
            return value;
        }
    };

    struct control_block {
        promise_type* promise;
        std::atomic<int> ref_count{ 1 };
        std::mutex mtx;

        explicit control_block(promise_type* promise) : promise(promise) {}
    };

    explicit thread_safe_coroutine(handle_type handle, std::shared_ptr<control_block> cb)
        : handle_(handle), cb_(std::move(cb)) {}

    ~thread_safe_coroutine() {
        if (cb_) {
            std::unique_lock lk(cb_->mtx);
            if (--cb_->ref_count == 0) {
                if (handle_) {
                    handle_.destroy();
                    handle_ = nullptr;
                }
            }
        }
    }

    [[nodiscard]] bool is_ready() const noexcept {
        return cb_ && cb_->promise->state.load() == status::suspended;
    }

    [[nodiscard]] bool done() const noexcept {
        return !cb_ || cb_->promise->state.load() == status::completed;
    }

    Ty get() {
        if (!cb_) {
            throw std::runtime_error("Coroutine object empty");
        }

        std::unique_lock lk(cb_->promise->mtx);
        auto& state = cb_->promise->state;

        cb_->promise->cv.wait(
            lk, [&] { return state == status::suspended || state == status::completed; });

        if (state == status::completed) {
            if (cb_->promise->eptr) {
                std::rethrow_exception(cb_->promise->eptr);
            }
            return cb_->promise->get();
        }

        state.store(status::running);
        lk.unlock();

        handle_.resume();

        lk.lock();
        if (state == status::completed) {
            if (cb_->promise->eptr) {
                std::rethrow_exception(cb_->promise->eptr);
            }
        }
        return cb_->promise->get();
    }

    thread_safe_coroutine(const thread_safe_coroutine& other)
        : handle_(other.handle_), cb_(other.cb_) {
        if (cb_) {
            std::unique_lock lk(cb_->mtx);
            ++cb_->ref_count;
        }
    }

    thread_safe_coroutine& operator=(const thread_safe_coroutine& other) {
        if (this != &other) {
            if (cb_) {
                std::unique_lock lk(cb_->mtx);
                if (--cb_->ref_count == 0) {
                    if (handle_) {
                        handle_.destroy();
                        handle_ = nullptr;
                    }
                }
            }

            handle_ = other.handle_;
            cb_     = other.cb_;

            if (cb_) {
                std::unique_lock lk(cb_->mtx);
                ++cb_->ref_count;
            }
        }
        return *this;
    }

    thread_safe_coroutine(thread_safe_coroutine&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)), cb_(std::exchange(other.cb_, nullptr)) {}

    thread_safe_coroutine& operator=(thread_safe_coroutine&& other) noexcept {
        if (this != &other) {
            if (cb_) {
                std::unique_lock lk(cb_->mtx);
                if (--cb_->ref_count == 0) {
                    if (handle_) {
                        handle_.destroy();
                    }
                }
            }

            handle_ = std::exchange(other.handle_, nullptr);
            cb_     = std::exchange(other.cb_, nullptr);
        }
        return *this;
    }

private:
    handle_type handle_;
    std::shared_ptr<control_block> cb_;
};

} // namespace neutron
