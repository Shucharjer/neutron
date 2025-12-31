// IWYU pragma: private, include "neutron/memory.hpp"
#pragma once
#include <exception> // IWYU pragma: keep
#include <iterator>
#include <memory>
#include <type_traits>

namespace neutron {

template <std::input_iterator InputIt, std::forward_iterator NothrowForwardIt>
constexpr auto uninitialized_move_if_noexcept(
    InputIt first, InputIt last, NothrowForwardIt dst) {
    using value_type = std::iter_value_t<InputIt>;
    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        return std::uninitialized_move(first, last, dst);
    } else {
        return std::uninitialized_copy(first, last, dst);
    }
}

template <
    std::input_iterator InputIt, typename Size,
    std::forward_iterator NothrowForwardIt>
constexpr auto uninitialized_move_if_noexcept_n(
    InputIt first, Size count, NothrowForwardIt dst) -> NothrowForwardIt {
    using value_type = std::iter_value_t<InputIt>;
    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        auto [_, result] = std::uninitialized_move_n(first, count, dst);
        return result;
    } else {
        return std::uninitialized_copy_n(first, count, dst);
    }
}

template <
    typename Exec, std::input_iterator InputIt,
    std::forward_iterator NothrowForwardIt>
constexpr auto uninitialized_move_if_noexcept(
    Exec&& exec, InputIt first, InputIt last, NothrowForwardIt dst) {
    using value_type = std::iter_value_t<InputIt>;
    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        return std::uninitialized_move(
            std::forward<Exec>(exec), first, last, dst);
    } else {
        return std::uninitialized_copy(
            std::forward<Exec>(exec), first, last, dst);
    }
}

template <
    typename Exec, std::input_iterator InputIt, typename Size,
    std::forward_iterator NothrowForwardIt>
constexpr auto uninitialized_move_if_noexcept_n(
    Exec&& exec, InputIt first, Size count, NothrowForwardIt dst)
    -> NothrowForwardIt {
    using value_type = std::iter_value_t<InputIt>;
    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        auto [_, result] = std::uninitialized_move_n(
            std::forward<Exec>(exec), first, count, dst);
        return result;
    } else {
        return std::uninitialized_copy_n(
            std::forward<Exec>(exec), first, count, dst);
    }
}

} // namespace neutron
