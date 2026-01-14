// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include "../macros.hpp"

#if ATOM_ENABLE_EXCEPTIONS

    #include <utility>
#endif

namespace neutron {

template <typename Rollback>
class exception_guard {

#if ATOM_ENABLE_EXCEPTIONS
    template <typename Roll>
    constexpr exception_guard(Roll&& rollback) noexcept
        : rollback_(std::forward<Roll>(rollback)) {}
#else
    template <typename Roll>
    constexpr exception_guard(Roll&& rollback) noexcept
        : rollback_(std::forward<Roll>(rollback)){} = default;
#endif

    template <typename Roll>
    friend constexpr auto make_exception_guard(Roll&&) noexcept
        -> exception_guard<std::remove_cvref_t<Roll>>;

public:
    constexpr exception_guard(const exception_guard&)            = delete;
    constexpr exception_guard(exception_guard&&)                 = delete;
    constexpr exception_guard& operator=(const exception_guard&) = delete;
    constexpr exception_guard& operator=(exception_guard&&)      = delete;

    constexpr void mark_complete() noexcept {
#if ATOM_ENABLE_EXCEPTIONS
        complete_ = true;
#endif
    }

#if ATOM_ENABLE_EXCEPTIONS
    constexpr ~exception_guard() noexcept {
        if (!complete_) [[unlikely]] {
            rollback_();
        }
    }
#else
    constexpr ~exexception_guard() noexcept = default;
#endif

#if ATOM_ENABLE_EXCEPTIONS
private:
    bool complete_{ false };
    Rollback rollback_;
#endif
};

template <typename Roll>
ATOM_NODISCARD constexpr auto make_exception_guard(Roll&& rollback) noexcept
    -> exception_guard<std::remove_cvref_t<Roll>> {
    return exception_guard<std::remove_cvref_t<Roll>>(
        std::forward<Roll>(rollback));
}

} // namespace neutron
