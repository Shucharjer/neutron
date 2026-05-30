// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/excluding.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

struct null_gurad {
    constexpr null_gurad() noexcept = default;

    template <excluding<null_gurad> Ignored>
    constexpr null_gurad(Ignored&&) noexcept {}

    ATOM_FORCE_INLINE constexpr void dismiss() noexcept {}
};

template <typename Rollback>
class completion_guard {
    template <typename Roll>
    constexpr completion_guard(Roll&& rollback) noexcept
        : rollback_(std::forward<Roll>(rollback)) {}

    template <typename Roll>
    friend constexpr auto make_completion_guard(Roll&&) noexcept
        -> completion_guard<std::remove_cvref_t<Roll>>;

    template <bool Noexcept, typename Roll>
    friend constexpr auto make_completion_guard(Roll&&) noexcept
        -> std::conditional_t<
            Noexcept, null_gurad, completion_guard<std::decay_t<Roll>>>;

public:
    static_assert(
        std::is_nothrow_invocable_v<Rollback>,
        "rollback function should be nothrow");

    constexpr completion_guard(const completion_guard&)            = delete;
    constexpr completion_guard(completion_guard&&)                 = delete;
    constexpr completion_guard& operator=(const completion_guard&) = delete;
    constexpr completion_guard& operator=(completion_guard&&)      = delete;

    constexpr void dismiss() noexcept { dismissed_ = true; }

    constexpr ~completion_guard() noexcept {
        if (!dismissed_) [[unlikely]] {
            rollback_();
        }
    }

private:
    bool dismissed_{ false };
    Rollback rollback_;
};

template <typename Roll>
ATOM_NODISCARD constexpr auto make_completion_guard(Roll&& rollback) noexcept
    -> completion_guard<std::remove_cvref_t<Roll>> {
    return completion_guard<std::remove_cvref_t<Roll>>(
        std::forward<Roll>(rollback));
}

#if ATOM_ENABLE_EXCEPTIONS

template <typename Roll>
ATOM_NODISCARD constexpr auto make_exception_guard(Roll&& rollback) noexcept
    -> completion_guard<std::remove_cvref_t<Roll>> {
    return make_completion_guard(std::forward<Roll>(rollback));
}

template <bool Noexcept, typename Roll>
ATOM_NODISCARD constexpr auto make_exception_guard(Roll&& rollback) noexcept
    -> std::conditional_t<
        Noexcept, null_gurad, completion_guard<std::decay_t<Roll>>> {
    if constexpr (Noexcept) {
        return {};
    } else {
        return make_completion_guard(std::forward<Roll>(rollback));
    }
}

#else

template <typename Roll>
constexpr null_gurad make_exception_guard(Roll&&) noexcept {
    return {};
}

template <bool Noexcept, typename Roll>
constexpr null_gurad make_exception_guard(Roll&&) noexcept {
    return {};
}

#endif

} // namespace neutron
