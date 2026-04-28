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
#include "neutron/detail/execution/coroutine_utilities/affine.hpp"
#include "neutron/detail/execution/coroutine_utilities/as_awaitable.hpp"
#include "neutron/detail/execution/coroutine_utilities/inline_scheduler.hpp"
#include "neutron/detail/execution/coroutine_utilities/task_scheduler.hpp"
#include "neutron/detail/execution/coroutine_utilities/with_awaitable_senders.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/queries.hpp"
#include "neutron/detail/execution/queries/get_env.hpp"
#include "neutron/detail/execution/queries/get_scheduler.hpp"
#include "neutron/detail/execution/receivers/set_error.hpp"
#include "neutron/detail/execution/receivers/set_stopped.hpp"
#include "neutron/detail/execution/receivers/set_value.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/has.hpp"
#include "neutron/detail/stop_token/inplace_stop_source.hpp"

namespace neutron::execution {

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

template <typename Env>
struct _env_errors {
    using type = completion_signatures<set_error_t(std::exception_ptr)>;
};
template <typename Env>
requires requires { typename Env::error_types; }
struct _env_errors<Env> {
    using type = typename Env::error_types;
};

template <typename T>
struct _set_value_type {
    using type = set_value_t(T);
};
template <>
struct _set_value_type<void> {
    using type = set_value_t();
};

template <typename T>
struct _final_awaitable {
    constexpr bool await_ready() noexcept { return false; } // NOLINT

    template <typename Promise>
    constexpr void
        await_suspend(std::coroutine_handle<Promise> handle) noexcept {
        /*
        if err then set_error
        else set_value
        endif
        handle.resume
        */
    }

    unspecified<void> await_resume() noexcept {}
};

template <typename T>
struct _promise_base {
    template <typename Val>
    constexpr void return_value(Val&& val) noexcept(
        std::is_nothrow_constructible_v<T, Val>) {
        _result.emplace(std::forward<Val>(val));
    }

    ATOM_NODISCARD constexpr T& result() noexcept { return *_result; }

    std::optional<T> _result;
};
template <>
class _promise_base<void> {
public:
    constexpr void return_void() const noexcept {}

    constexpr void result() const noexcept {}
};

template <typename Env, typename Rcvr>
struct _own_env {
    using type = env<>;
};
template <typename Env, typename Rcvr>
requires requires {
    typename Env::template env_type<decltype(get_env(std::declval<Rcvr>()))>;
}
struct _own_env<Env, Rcvr> {
    using type = typename Env::template env_type<decltype(get_env(
        std::declval<Rcvr>()))>;
};

} // namespace _task

template <typename T = void, typename Env = env<>>
class ATOM_NODISCARD task {
    template <receiver Rcvr>
    class _state;

    static_assert(
        std::is_void_v<T> || std::is_reference_v<T> || !std::is_array_v<T>,
        "T must be void, reference type or cv-unqualified non-array object type");

    static_assert(std::is_class_v<Env>);

public:
    using sender_concept   = sender_tag;
    using allocator_type   = typename _task::_env_allocator<Env>::type;
    using scheduler_type   = typename _task::_env_scheduler<Env>::type;
    using stop_source_type = typename _task::_env_stop_source<Env>::type;
    using stop_token_type =
        decltype(std::declval<stop_source_type>().get_token());
    using error_types = _task::_env_errors<Env>::type;

    using completion_signatures = ::neutron::execution::completion_signatures<
        typename _task::_set_value_type<T>::type,
        /* set errors for err in error_types */
        set_stopped_t()>;

    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct _opstate_base {
        constexpr _opstate_base(scheduler_type sched) noexcept
            : sched_(sched) {}
        scheduler_type sched_;
    };

    class promise_type :
        public _task::_promise_base<T>,
        public with_awaitable_senders<promise_type> {
    public:
        template <typename... Args>
        promise_type(const Args&... args);

        ATOM_NODISCARD task get_return_object() noexcept {
            return task{ handle_type::from_promise(*this) };
        }

        static constexpr std::suspend_always initial_suspend() noexcept {
            return {};
        }

        constexpr auto final_suspend() noexcept -> _task::_final_awaitable<T> {
            return {};
        }

        void unhandled_exception() {
            errors_.template emplace<1>(std::current_exception());
        }

        ATOM_NODISCARD std::coroutine_handle<> unhandled_stopped() noexcept {
            // TODO: Finish this function. This function is not useable
            std::terminate();
            return std::noop_coroutine();
        }

        template <class E>
        constexpr unspecified<> yield_value(with_error<E> error) {
            if constexpr (type_list_has_v<error_types, E>) {

            } else {
                static_assert(false, "Error type not in task's error_types");
            }
        }

        template <sender Sndr>
        auto await_transform(Sndr&& sndr) noexcept {
            if constexpr (std::same_as<inline_scheduler, scheduler_type>) {
                return as_awaitable(std::forward<Sndr>(sndr), *this);
            } else {
                return as_awaitable(affine(std::forward<Sndr>(sndr)), *this);
            }
        }
        template <class Sch>
        auto await_transform(change_coroutine_scheduler<Sch> sch) noexcept {
            return await_transform(
                just(
                    std::exchange(
                        _sched(*this), scheduler_type(sch.scheduler))),
                *this);
        }

        struct _env {
            const promise_type* promise;

            constexpr auto query(get_scheduler_t) const noexcept {
                return promise->state_->sched_;
            }

            constexpr auto query(get_allocator_t) const noexcept {
                return promise->alloc_;
            }

            constexpr auto query(get_stop_token_t) const noexcept {
                return promise->token_;
            }

            // template <typename Query, typename... Args>
            // constexpr auto query(Query qry, Args... args) const noexcept
            // requires requires { forwarding_query(qry); }
            // {
            //     return STATE(*this).environment.query(qry, args...);
            // }
        };

        ATOM_NODISCARD constexpr _env get_env() const noexcept {
            return _env{ this };
        }

        template <class... Args>
        void* operator new(size_t size, Args&&... args);

        void operator delete(void* pointer, size_t size) noexcept;

    private:
        template <typename>
        struct _error_convert;
        template <typename E>
        struct _error_convert<set_error_t(E)> {
            using type = std::remove_cvref_t<E>;
        };
        using _error_variant = type_list_cat_t<
            std::variant<std::monostate>,
            type_list_convert_t<
                _error_convert, type_list_rebind_t<std::variant, error_types>>>;

        allocator_type alloc_;
        stop_source_type source_;
        stop_token_type token_;
        _error_variant errors_;
        _opstate_base* state_;
    };

    constexpr task(task&& that) noexcept
        : handle_(std::exchange(that.handle_, {})) {}

    task(const task&)            = delete;
    task& operator=(const task&) = delete;
    task& operator=(task&&)      = delete;

    constexpr explicit task(handle_type handle) noexcept : handle_(handle) {}

    constexpr ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    template <receiver Rcvr>
    constexpr _state<Rcvr> connect(Rcvr&& rcvr) && {
        return _state<Rcvr>(
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
    class _state : _opstate_base {
    public:
        using operation_state_concept = operation_state_tag;

        template <typename R>
        _state(std::coroutine_handle<promise_type> handle, R&& rcvr)
        requires requires { _own_env_t(get_env(rcvr)); }
            : handle_(std::move(handle)), rcvr_(std::forward<R>(rcvr)),
              own_env_(get_env(rcvr)) {}

        template <typename R>
        _state(std::coroutine_handle<promise_type> handle, R&& rcvr)
        requires(!requires { _own_env_t(get_env(rcvr)); })
            : _opstate_base(_make_sched(rcvr)), handle_(std::move(handle)),
              rcvr_(std::forward<R>(rcvr)), own_env_() {}

        ~_state() {
            if (handle_) {
                handle_.destroy();
            }
        }

        void start() & noexcept {
            //
        }

    private:
        constexpr auto _make_sched(const Rcvr& rcvr) {
            if constexpr (requires {
                              scheduler_type(get_scheduler(
                                  ::neutron::execution::get_env(rcvr)));
                          }) {
                return scheduler_type(
                    get_scheduler(::neutron::execution::get_env(rcvr)));
            } else {
                return scheduler_type{};
            }
        }

        using _own_env_t = typename _task::_own_env<Env, Rcvr>::type;
        std::coroutine_handle<promise_type> handle_;
        std::remove_cvref_t<Rcvr> rcvr_;
        _own_env_t own_env_;
        Env env_;
    };

    std::coroutine_handle<promise_type> handle_;
};

static_assert(sender<task<void>>);
static_assert(sender<task<int>>);

} // namespace neutron::execution
