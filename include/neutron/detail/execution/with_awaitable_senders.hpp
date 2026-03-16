#pragma once
#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/as_awaitable.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

template <_class_type Promise>
struct with_awaitable_senders {
    template <class OtherPromise>
    requires(!std::same_as<OtherPromise, void>)
    void set_continuation(std::coroutine_handle<OtherPromise> handle) noexcept {
        continuation_ = handle;
        if constexpr (requires(OtherPromise& other) {
                          other.unhandled_stopped();
                      }) {
            stopped_handler_ =
                [](void* promise) noexcept -> std::coroutine_handle<> {
                return std::coroutine_handle<OtherPromise>::from_address(
                    promise);
            };
        } else {
            stopped_handler_ = &_default_unhandled_stopped;
        }
    }

    ATOM_NODISCARD std::coroutine_handle<> continuation() const noexcept {
        return continuation_;
    }

    std::coroutine_handle<> unhandled_stopped() noexcept {
        return stopped_handler_(continuation_.address());
    }

    template <typename Value>
    auto await_transform(Value&& value)
        -> std::invoke_result_t<as_awaitable_t, Value, Promise&> {
        return as_awaitable(
            std::forward<Value>(value), static_cast<Promise&>(*this));
    }

private:
    [[noreturn]] static std::coroutine_handle<>
        _default_unhandled_stopped(void*) noexcept { // exposition only
        std::terminate();
    }

    using handle_t = std::coroutine_handle<>;
    handle_t continuation_;                        // exposition only
    handle_t (*stopped_handler_)(void*) noexcept = // exposition only
        &_default_unhandled_stopped;
};

} // namespace neutron::execution
