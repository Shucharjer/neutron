// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#include <cstring>
#include <exception> // IWYU pragma: keep
#include <iterator>
#include <memory>
#include <type_traits>
#include <neutron/concepts.hpp>
#include "neutron/detail/concepts/trivially_relocatable.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

template <std::input_iterator InputIt, std::forward_iterator NothrowForwardIter>
ATOM_CONSTEXPR_SINCE_CXX26 auto uninitialized_move_if_noexcept(
    InputIt first, InputIt last,
    NothrowForwardIter
        dst) noexcept(nothrow_conditional_movable<std::iter_value_t<InputIt>>)
    -> NothrowForwardIter {
    using value_type = std::iter_value_t<InputIt>;
#if __cplusplus < 202602L
    if constexpr (
        std::contiguous_iterator<InputIt> &&
        std::contiguous_iterator<NothrowForwardIter> &&
        trivially_relocatable<value_type>) {
        if ATOM_CONST_EVALUATED {
            return std::uninitialized_move(first, last, dst);
        }

        const auto dist = std::distance(first, last);
        std::memcpy(
            std::addressof(*dst), std::addressof(*first),
            sizeof(value_type) * dist);
        return dst + dist;
    }
#endif

    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        return std::uninitialized_move(first, last, dst);
    }

    return std::uninitialized_copy(first, last, dst);
}

template <
    std::input_iterator InputIt, typename Size,
    std::forward_iterator NothrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX26 auto uninitialized_move_if_noexcept_n(
    InputIt first, Size count, NothrowForwardIt dst) -> NothrowForwardIt {
    using value_type = std::iter_value_t<InputIt>;
#if __cplusplus < 202602L
    if constexpr (
        std::contiguous_iterator<InputIt> &&
        std::contiguous_iterator<NothrowForwardIt> &&
        trivially_relocatable<value_type>) {
        if ATOM_CONST_EVALUATED {
            auto [_, result] = std::uninitialized_move_n(first, count, dst);
            return result;
        }

        std::memcpy(
            std::addressof(*dst), std::addressof(*first),
            sizeof(value_type) * count);
        return dst + count;
    }
#endif

    if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
        auto [_, result] = std::uninitialized_move_n(first, count, dst);
        return result;
    }

    return std::uninitialized_copy_n(first, count, dst);
}

template <
    typename Exec, std::input_iterator InputIt,
    std::forward_iterator NothrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX26 auto uninitialized_move_if_noexcept(
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
ATOM_CONSTEXPR_SINCE_CXX26 auto uninitialized_move_if_noexcept_n(
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
