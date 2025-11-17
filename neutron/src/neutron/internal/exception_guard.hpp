#pragma once

#if _MSC_VER
    #include <vcruntime.h> // for _HAS_EXCEPTIONS
#endif
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || (_HAS_EXCEPTIONS)
    #define ENABLED_CPP_EXCEPTIONS true
#else
    #define ENABLED_CPP_EXCEPTIONS false
#endif

#include "macros.hpp"

#if ENABLED_CPP_EXCEPTIONS

    #include <utility>

namespace neutron {

template <typename Rollback>
class exception_guard {

    template <typename Roll>
    constexpr exception_guard(Roll&& rollback) noexcept
        : rollback_(std::forward<Roll>(rollback)) {}

    template <typename Roll>
    friend constexpr auto make_exception_guard(Roll&&) noexcept
        -> exception_guard<std::remove_cvref_t<Roll>>;

public:
    constexpr exception_guard(const exception_guard&)            = delete;
    constexpr exception_guard(exception_guard&&)                 = delete;
    constexpr exception_guard& operator=(const exception_guard&) = delete;
    constexpr exception_guard& operator=(exception_guard&&)      = delete;

    constexpr void mark_complete() noexcept { complete_ = true; }

    constexpr ~exception_guard() noexcept {
        if (!complete_) [[unlikely]] {
            rollback_();
        }
    }

private:
    bool complete_{ false };
    Rollback rollback_;
};

template <typename Roll>
NODISCARD constexpr auto make_exception_guard(Roll&& rollback) noexcept
    -> exception_guard<std::remove_cvref_t<Roll>> {
    return exception_guard<std::remove_cvref_t<Roll>>(
        std::forward<Roll>(rollback));
}

} // namespace neutron

#else

class _empty_exception_guard {
    constexpr exception_guard() noexcept = default;

    template <typename Rollback>
    friend constexpr auto make_exception_guard(Rollback&& rollback) noexcept
        -> _empty_exception_guard;

public:
    constexpr exception_guard(const exception_guard&)            = delete;
    constexpr exception_guard(exception_guard&&)                 = delete;
    constexpr exception_guard& operator=(const exception_guard&) = delete;
    constexpr exception_guard& operator=(exception_guard&&)      = delete;

    constexpr void mark_complete() noexcept {}

    constexpr ~exception_guard() noexcept = default;
};

template <typename Rollback>
NODISCARD auto make_exception_guard(Rollback&& rollback) noexcept
    -> _empty_exception_guard {
    return _empty_exception_guard{};
}

#endif
