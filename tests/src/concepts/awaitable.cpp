#include <coroutine>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Promise type without `await_transform`.
struct promise {};

/// Minimal awaiter, `await_suspend` returning void.
struct void_awaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept { return 1; }
};

/// `await_suspend` returning bool, `await_ready` returning a convertible.
struct bool_awaiter {
    struct flag {
        operator bool() const noexcept { return true; }
    };
    flag await_ready() noexcept { return {}; }
    bool await_suspend(std::coroutine_handle<promise>) noexcept { return true; }
    void await_resume() noexcept {}
};

/// Symmetric transfer.
struct handle_awaiter {
    bool await_ready() noexcept { return false; }
    std::coroutine_handle<promise>
        await_suspend(std::coroutine_handle<promise> handle) noexcept {
        return handle;
    }
    void await_resume() noexcept {}
};

/// Accepts only handles of `promise`.
struct typed_awaiter {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<promise>) noexcept {}
    void await_resume() noexcept {}
};

/// Non-const members.
struct mutable_awaiter {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() noexcept {}
};

/// `await_suspend` returning `int`, which is not a valid result.
struct invalid_suspend_awaiter {
    bool await_ready() noexcept { return false; }
    int await_suspend(std::coroutine_handle<>) noexcept { return 0; }
    void await_resume() noexcept {}
};

/// `await_suspend` returning `bool&` instead of `bool`.
struct bool_ref_suspend_awaiter {
    bool flag = false;
    bool await_ready() noexcept { return false; }
    bool& await_suspend(std::coroutine_handle<>) noexcept { return flag; }
    void await_resume() noexcept {}
};

/// Missing `await_ready`.
struct no_ready_awaiter {
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() noexcept {}
};

/// Missing `await_resume`.
struct no_resume_awaiter {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
};

/// `await_ready` returning something not convertible to bool.
struct non_bool_ready_awaiter {
    struct none {};
    none await_ready() noexcept { return {}; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() noexcept {}
};

/// Awaitable through a member `operator co_await`.
struct member_co_await {
    void_awaiter operator co_await() const noexcept { return {}; }
};

/// Awaitable through a free `operator co_await` found by ADL.
struct free_co_await {};
void_awaiter operator co_await(free_co_await) noexcept { return {}; }

/// Member `operator co_await` available only for rvalues.
struct rvalue_co_await {
    void_awaiter operator co_await() && noexcept { return {}; }
};

/// `operator co_await` yielding something that is not an awaiter.
struct bad_co_await {
    int operator co_await() const noexcept { return 0; }
};

/// Not awaitable by itself; only promises with `await_transform` accept it.
struct token {};

/// Promise transforming `token` into an awaiter.
struct transforming_promise {
    void_awaiter await_transform(token) noexcept { return {}; }
};

/// Promise transforming `token` into a member co_await awaitable.
struct co_await_transforming_promise {
    member_co_await await_transform(token) noexcept { return {}; }
};

/// Promise transforming `token` into an ADL co_await awaitable.
struct free_co_await_transforming_promise {
    free_co_await await_transform(token) noexcept { return {}; }
};

// Distinct decimal digits, so the sum shows which awaiters were resumed.
constexpr int member_value      = 1;
constexpr int free_value        = 10;
constexpr int transformed_value = 100;
constexpr int direct_value      = 1000;

/// Never suspends, yields a value.
struct ready_awaiter {
    int value;
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept { return value; }
};

/// Yields `member_value` through a member `operator co_await`.
struct ready_member {
    ready_awaiter operator co_await() const noexcept {
        return { member_value };
    }
};

/// Yields `free_value` through a free `operator co_await`.
struct ready_free {};
ready_awaiter operator co_await(ready_free) noexcept { return { free_value }; }

/// Promise members shared by both `eager` coroutine types.
struct eager_promise_base {
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
};

/// Eager coroutine that runs to completion without suspending.
template <bool Transform>
struct eager {
    struct promise_type : eager_promise_base {
        eager get_return_object() noexcept { return {}; }
    };
};

/// Eager coroutine whose promise transforms `token` into a ready awaiter.
template <>
struct eager<true> {
    struct promise_type : eager_promise_base {
        eager get_return_object() noexcept { return {}; }
        ready_awaiter await_transform(token) noexcept {
            return { transformed_value };
        }
    };
};

using plain_promise     = eager<false>::promise_type;
using transform_promise = eager<true>::promise_type;

// await_suspend_result: satisfied by
static_assert(await_suspend_result<void>);
static_assert(await_suspend_result<bool>);
static_assert(await_suspend_result<std::coroutine_handle<>>);
static_assert(await_suspend_result<std::coroutine_handle<promise>>);
static_assert(
    await_suspend_result<std::coroutine_handle<std::noop_coroutine_promise>>);

// await_suspend_result: rejected
static_assert(!await_suspend_result<int>);
static_assert(!await_suspend_result<promise>);

// await_suspend_result: rejected: references and cv-qualified types
static_assert(!await_suspend_result<bool&>);
static_assert(!await_suspend_result<const bool>);
static_assert(!await_suspend_result<std::coroutine_handle<>&>);

// awaiter: satisfied by
static_assert(awaiter<std::suspend_always, void>);
static_assert(awaiter<std::suspend_never, promise>);
static_assert(awaiter<void_awaiter, void>);
static_assert(awaiter<void_awaiter, promise>);
static_assert(awaiter<const void_awaiter, promise>);
static_assert(awaiter<void_awaiter&, promise>);
static_assert(awaiter<bool_awaiter, promise>);
static_assert(awaiter<handle_awaiter, promise>);
static_assert(awaiter<typed_awaiter, promise>);
static_assert(awaiter<mutable_awaiter, promise>);

// awaiter: rejected: handle of another promise type
static_assert(!awaiter<typed_awaiter, void>);
static_assert(!awaiter<typed_awaiter, transforming_promise>);

// awaiter: rejected: non-const members on a const awaiter
static_assert(!awaiter<const mutable_awaiter, promise>);

// awaiter: rejected: missing or ill-typed members
static_assert(!awaiter<invalid_suspend_awaiter, promise>);
static_assert(!awaiter<bool_ref_suspend_awaiter, promise>);
static_assert(!awaiter<no_ready_awaiter, promise>);
static_assert(!awaiter<no_resume_awaiter, promise>);
static_assert(!awaiter<non_bool_ready_awaiter, promise>);

// awaiter: rejected: awaitables that are not awaiters themselves
static_assert(!awaiter<member_co_await, promise>);
static_assert(!awaiter<int, promise>);

// awaitable without await_transform: satisfied by
static_assert(awaitable<std::suspend_always, void>);
static_assert(awaitable<void_awaiter, promise>);
static_assert(awaitable<void_awaiter&, promise>);
static_assert(awaitable<member_co_await, promise>);
static_assert(awaitable<const member_co_await&, promise>);
static_assert(awaitable<free_co_await, promise>);
static_assert(awaitable<rvalue_co_await, promise>);
static_assert(awaitable<typed_awaiter, promise>);

// awaitable without await_transform: rejected
static_assert(!awaitable<rvalue_co_await&, promise>);
static_assert(!awaitable<bad_co_await, promise>);
static_assert(!awaitable<typed_awaiter, void>);
static_assert(!awaitable<int, promise>);
static_assert(!awaitable<token, promise>);

// awaitable through await_transform: satisfied by
static_assert(awaitable<token, transforming_promise>);
static_assert(awaitable<token, co_await_transforming_promise>);
static_assert(awaitable<token, free_co_await_transforming_promise>);

// awaitable through await_transform: rejected without a promise
static_assert(!awaitable<token, void>);

// real coroutine promises used by the runtime tests
static_assert(awaitable<ready_awaiter, plain_promise>);
static_assert(awaitable<ready_member, plain_promise>);
static_assert(awaitable<ready_free, plain_promise>);
static_assert(!awaitable<token, plain_promise>);
static_assert(awaitable<token, transform_promise>);

eager<false> run_plain(int& out) {
    out += co_await ready_awaiter{ direct_value };
    out += co_await ready_member{};
    out += co_await ready_free{};
}

eager<true> run_transform(int& out) { out += co_await token{}; }

void test_co_await_resumes_with_awaiter_values() {
    int out = 0;

    run_plain(out);

    require(out == direct_value + member_value + free_value);
}

void test_await_transform_supplies_the_awaiter() {
    int out = 0;

    run_transform(out);

    require(out == transformed_value);
}

} // namespace

int main() {
    test_co_await_resumes_with_awaiter_values();
    test_await_transform_supplies_the_awaiter();
    return neutron_test::result();
}
