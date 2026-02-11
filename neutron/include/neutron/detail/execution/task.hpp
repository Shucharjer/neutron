#pragma once
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/set_stopped.hpp"
#include "neutron/detail/execution/set_value.hpp"

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

template <typename T = void, typename Env = empty_env>
class task {
    template <receiver Rcvr>
    class state;

public:
    using sender_concept   = sender_t;
    using allocator_type   = Env::allocator_type;
    using scheduler_type   = Env::scheduler_type;
    using stop_source_type = Env::stop_source_type;
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

        auto final_suspend() noexcept {}

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
