#pragma once
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <neutron/concepts.hpp>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/inplace_stop_source.hpp"
#include "neutron/detail/execution/set_stopped.hpp"
#include "neutron/detail/execution/set_value.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

class inline_scheduler {
    class _inline_sender;

    template <receiver Rcvr>
    class _inline_state;

public:
    using scheduler_concept = scheduler_t;
    constexpr _inline_sender schedule() noexcept;
    constexpr bool operator==(const inline_scheduler&) const noexcept = default;
};

class task_scheduler {
    class _sender;

    template <receiver Rcvr>
    class _state;

public:
    using scheduler_concept = scheduler_t;

    template <class Sch, class Allocator = std::allocator<void>>
    requires(!std::same_as<task_scheduler, std::remove_cvref_t<Sch>>) &&
            scheduler<Sch>
    explicit task_scheduler(Sch&& sch, Allocator alloc = {});

    task_scheduler(const task_scheduler&)            = default;
    task_scheduler& operator=(const task_scheduler&) = default;

    _sender schedule();

    friend bool operator==(
        const task_scheduler& lhs, const task_scheduler& rhs) noexcept;
    template <class Sch>
    requires(!std::same_as<task_scheduler, Sch>) && scheduler<Sch>
    friend bool operator==(const task_scheduler& lhs, const Sch& rhs) noexcept;

private:
    std::shared_ptr<void> sch_;
};

template <typename T = void>
using unspecified = T;

template <class E>
struct with_error {
    using type = std::remove_cvref_t<E>;
    type error;
};
template <class E>
with_error(E) -> with_error<E>;

template <scheduler Sch>
struct change_coroutine_scheduler {
    using type = std::remove_cvref_t<Sch>;
    type scheduler;
};
template <scheduler Sch>
change_coroutine_scheduler(Sch) -> change_coroutine_scheduler<Sch>;

namespace _task {

template <typename Env>
struct _env_allocator {
    using type = std::allocator<std::byte>;
};
template <typename Env>
requires requires { typename Env::allocator_type; }
struct _env_allocator<Env> {
    using type = Env::allocator_type;
};

template <typename Env>
struct _env_scheduler {
    using type = task_scheduler;
};
template <typename Env>
requires requires { typename Env::scheduler_type; }
struct _env_scheduler<Env> {
    using type = Env::scheduler_type;
};

template <typename Env>
struct _env_stop_source {
    using type = inplace_stop_source;
};
template <typename Env>
requires requires { typename Env::scheduler_type; }
struct _env_stop_source<Env> {
    using type = Env::stop_source_type;
};

} // namespace _task

template <typename T = void, typename Env = empty_env>
class task {
    template <receiver Rcvr>
    class state;

public:
    using sender_concept   = sender_t;
    using allocator_type   = typename _task::_env_allocator<Env>::type;
    using scheduler_type   = typename _task::_env_scheduler<Env>::type;
    using stop_source_type = typename _task::_env_stop_source<Env>::type;
    using stop_token_type =
        decltype(std::declval<stop_source_type>().get_token());
    using error_types = Env::error_types;

    using completion_signatures = ::neutron::execution::completion_signatures<
        std::conditional_t<
            std::same_as<T, void>, set_value_t(), set_value_t(T)>,
        /* set errors for err in error_types */
        set_stopped_t()>;

    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    class promise_type {
    public:
        template <typename... Args>
        promise_type(const Args&... args);

        task get_return_object() noexcept {
            return task{ handle_type::from_promise(*this) };
        }

        static constexpr std::suspend_always initial_suspend() noexcept {
            return {};
        }

        struct _final_awaitable {
            constexpr bool await_ready() noexcept { return false; }

            template <typename Promise>
            constexpr auto
                await_suspend(std::coroutine_handle<Promise> handle) noexcept {
                return handle.promise().continuation;
            }

            unspecified<> await_resume() noexcept {
                // TODO
            }
        };

        constexpr auto final_suspend() noexcept -> _final_awaitable {
            return {};
        }

        void unhandled_exception() {
            errors_.template emplace<1>(std::current_exception());
        }

        std::coroutine_handle<> unhandled_stopped() noexcept {}

        void return_void()
        requires std::is_void_v<T> /* present only if is_void_v<T> is true */ {}

        template <class V = T>
        requires std::negation_v<std::is_void<T>>
        void return_value(V&& value)
        // present only if is_void_v<T> is false
        {
            result_.emplace(std::forward<V>(value));
        }

        template <class E>
        unspecified<> yield_value(with_error<E> error);

        template <class A>
        auto await_transform(A&& a);
        template <class Sch>
        auto await_transform(change_coroutine_scheduler<Sch> sch);

        unspecified<> get_env() const noexcept;

        template <class... Args>
        void* operator new(size_t size, Args&&... args);

        void operator delete(void* pointer, size_t size) noexcept;

    private:
        using _error_variant = type_list_cat_t<
            std::variant<std::monostate>,
            type_list_rebind_t<std::variant, std::remove_cvref_t<error_types>>>;

        allocator_type alloc_;
        stop_source_type source_;
        stop_token_type token_;
        std::optional<T> result_;
        _error_variant errors_;
    };

    task(task&& that) noexcept : handle_(std::exchange(that.handle_, {})) {}

    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    template <receiver Rcvr>
    state<Rcvr> connect(Rcvr&& rcvr) && {
        return state<Rcvr>(
            std::exchange(handle_, {}), std::forward<Rcvr>(rcvr));
    }

    struct _awaitable {
        ATOM_NODISCARD constexpr bool await_ready() const noexcept {
            return false;
        }

        template <typename Pro>
        auto await_suspend(std::coroutine_handle<Pro> handle) noexcept
            -> std::coroutine_handle<> {
            _handle.promise().set_continuation(handle);
            if constexpr (requires {
                              {
                                  _handle.stop_requested()
                              } -> std::convertible_to<bool>;
                          }) {
                if (_handle.promise().stop_requested()) {
                    handle.promise().unhandled_stopped();
                }
            }
            return _handle;
        }

        T await_resume() {
            if (!_handle) [[unlikely]] {
                throw std::runtime_error{ "broken promise" };
            }

            return std::move(_handle.promise());
        }

        void set_continuation(std::coroutine_handle<> hanele) noexcept {
            continuation = hanele;
        }

        std::coroutine_handle<promise_type> _handle;
        std::coroutine_handle<> continuation;
    };
    template <typename Promise>
    auto operator co_await() && noexcept {
        return _awaitable{ std::exchange(handle_, {}) };
    }

    operator bool() const noexcept { return handle_ && !handle_.done(); }

private:
    template <receiver Rcvr>
    class state {
        using _own_env_t = typename Env::template env_type<decltype(get_env(
            std::declval<Rcvr>()))>;

    public:
        using operation_state_concept = operation_state_t;

        template <typename R>
        state(std::coroutine_handle<promise_type> handle, R&& rcvr)
        requires requires { _own_env_t(get_env(rcvr)); }
            : handle_(std::move(handle)), rcvr_(std::forward<R>(rcvr)),
              own_env_(get_env(rcvr)) {}

        template <typename R>
        state(std::coroutine_handle<promise_type> handle, R&& rcvr)
        requires(!requires { _own_env_t(get_env(rcvr)); })
            : handle_(std::move(handle)), rcvr_(std::forward<R>(rcvr)),
              own_env_() {}

        ~state() {
            if (handle_) {
                handle_.destroy();
            }
        }

        void start() & noexcept;

    private:
        std::coroutine_handle<promise_type> handle_;
        std::remove_cvref_t<Rcvr> rcvr_;
        _own_env_t own_env_;
        Env env_;
    };

    std::coroutine_handle<promise_type> handle_;
};

} // namespace neutron::execution
