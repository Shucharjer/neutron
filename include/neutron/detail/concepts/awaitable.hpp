#pragma once
#include <concepts>
#include <coroutine>
#include <utility>
#include "neutron/detail/concepts/one_of.hpp"
#include "neutron/detail/metafn/specific.hpp"

namespace neutron {

template <typename Ty>
concept await_suspend_result =
    one_of<Ty, void, bool> || is_specific_type_list_v<std::coroutine_handle, Ty>;

template <typename Awaiter, typename Promise>
concept awaiter =
    requires(Awaiter& awaiter, std::coroutine_handle<Promise> handle) {
        { awaiter.await_ready() } -> std::convertible_to<bool>;
        { awaiter.await_suspend(handle) } -> await_suspend_result;
        awaiter.await_resume();
    };

namespace _awaitable {

template <typename Awaitable, typename Ignored>
decltype(auto) _get_awaiter(Awaitable&& awaitable, Ignored) {
    if constexpr (requires {
                      std::forward<Awaitable>(awaitable).operator co_await();
                  }) {
        return std::forward<Awaitable>(awaitable).operator co_await();
    } else if constexpr (requires {
                             operator co_await(
                                 std::forward<Awaitable>(awaitable));
                         }) {
        return operator co_await(std::forward<Awaitable>(awaitable));
    } else {
        return std::forward<Awaitable>(awaitable);
    }
};

template <typename Awaitable, typename Promise>
decltype(auto) _get_awaiter(Awaitable&& awaitable, Promise* promise)
requires requires {
    promise->await_transform(std::forward<Awaitable>(awaitable));
}
{
    if constexpr (requires {
                      promise
                          ->await_transform(std::forward<Awaitable>(awaitable))
                          .operator co_await();
                  }) {
        return promise->await_transform(std::forward<Awaitable>(awaitable))
            .operator co_await();
    } else if constexpr (requires {
                             operator co_await(promise->await_transform(
                                 std::forward<Awaitable>(awaitable)));
                         }) {
        return operator co_await(
            promise->await_transform(std::forward<Awaitable>(awaitable)));
    } else {
        return promise->await_transform(std::forward<Awaitable>(awaitable));
    }
};

} // namespace _awaitable

template <typename Awaitable, typename Promise>
concept awaitable = requires(Awaitable&& awaitable, Promise* promise) {
    {
        _awaitable::_get_awaiter(std::forward<Awaitable>(awaitable), promise)
    } -> awaiter<Promise>;
};

} // namespace neutron
