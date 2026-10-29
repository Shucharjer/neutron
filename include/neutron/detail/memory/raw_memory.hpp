// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#include <iterator>
#include <memory>
#include <utility>
#include <version>
#include "neutron/detail/iterator/concepts.hpp"

// NOTE: The raw memory algorithms in stl is constexpr since c++26.
// And each algo has no exception identifier.

#include "neutron/detail/utility/macros.hpp"
#if !defined(__cpp_if_consteval) || __cpp_if_consteval < 202106L
    #include <type_traits> // IWYU pragma: keep, mis-report
#endif

#if defined(__cpp_lib_raw_memory_algorithms) &&                                \
    __cpp_lib_raw_memory_algorithms >= 202411L
    #define NEUTRON_STL_CONSTEXPR_RAW_MEM_ALGOS true
#else
    #define NEUTRON_STL_CONSTEXPR_RAW_MEM_ALGOS false
#endif

namespace neutron {

namespace _raw_mem_algos {

template <typename It>
constexpr bool _n_default_construct =
    std::is_nothrow_default_constructible_v<std::iter_value_t<It>>;

template <typename It>
constexpr bool _n_value_construct = requires {
    { std::iter_value_t<It>() } noexcept;
};

// NOTE: involved into stl since c++23
template <typename It>
using iter_const_reference_t = std::common_reference_t<
    const std::iter_value_t<It>&&, std::iter_reference_t<It>>;

template <typename It, typename InputIt = It>
constexpr bool _n_copy = std::is_nothrow_constructible_v<
    std::iter_value_t<It>, iter_const_reference_t<InputIt>>;

template <typename It, typename InputIt = It>
constexpr bool _n_move = std::is_nothrow_constructible_v<
    std::iter_value_t<It>, std::iter_rvalue_reference_t<InputIt>>;

template <typename It, typename Val>
constexpr bool _n_fill =
    std::is_nothrow_constructible_v<std::iter_value_t<It>, Val>;

} // namespace _raw_mem_algos

template <nothrow_forward_iterator NoThrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_default_construct(
    NoThrowForwardIt first,
    NoThrowForwardIt
        last) noexcept(_raw_mem_algos::_n_default_construct<NoThrowForwardIt>)
    -> void {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        // NOTE: `construct_at` without arguments performs value-initialization.
        for (; first != last; ++first) {
            std::construct_at(std::addressof(*first));
        }
    }
#endif
    std::uninitialized_default_construct(first, last);
}

template <nothrow_forward_iterator NoThrowForwardIt, typename Size>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_default_construct_n(
    NoThrowForwardIt first,
    Size count) noexcept(_raw_mem_algos::_n_default_construct<NoThrowForwardIt>)
    -> NoThrowForwardIt {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        ;
        for (; count > 0; ++first, --count) {
            std::construct_at(std::addressof(*first));
        }
        return first;
    }
#endif
    return std::uninitialized_default_construct_n(first, count);
}

template <nothrow_forward_iterator NoThrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_value_construct(
    NoThrowForwardIt first,
    NoThrowForwardIt
        last) noexcept(_raw_mem_algos::_n_value_construct<NoThrowForwardIt>)
    -> void {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; first != last; ++first) {
            std::construct_at(std::addressof(*first));
        }
    }
#endif
    std::uninitialized_value_construct(first, last);
}

template <nothrow_forward_iterator NoThrowForwardIt, typename Size>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_value_construct_n(
    NoThrowForwardIt first,
    Size count) noexcept(_raw_mem_algos::_n_value_construct<NoThrowForwardIt>)
    -> NoThrowForwardIt {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; count > 0; ++first, --count) {
            std::construct_at(std::addressof(*first));
        }
        return first;
    }
#endif
    return std::uninitialized_value_construct_n(first, count);
}

template <input_iterator InputIt, nothrow_forward_iterator NoThrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_copy(
    InputIt first, InputIt last,
    NoThrowForwardIt
        d_first) noexcept(_raw_mem_algos::_n_copy<NoThrowForwardIt>)
    -> NoThrowForwardIt {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; first != last; ++first, ++d_first) {
            std::construct_at(std::addressof(*d_first), *first);
        }
        return d_first;
    }
#endif
    return std::uninitialized_copy(first, last, d_first);
}

template <
    input_iterator InputIt, typename Size,
    nothrow_forward_iterator NoThrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_copy_n(
    InputIt first, Size count,
    NoThrowForwardIt
        d_first) noexcept(_raw_mem_algos::_n_copy<NoThrowForwardIt>)
    -> NoThrowForwardIt {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; count > 0; ++first, ++d_first, --count) {
            std::construct_at(std::addressof(*d_first), *first);
        }
        return d_first;
    }
#endif
    return std::uninitialized_copy_n(first, count, d_first);
}

template <input_iterator InputIt, nothrow_forward_iterator NoThrowForwardIt>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_move(
    InputIt first, InputIt last,
    NoThrowForwardIt
        d_first) noexcept(_raw_mem_algos::_n_move<NoThrowForwardIt>)
    -> NoThrowForwardIt {
    return uninitialized_copy(
        std::make_move_iterator(first), std::make_move_iterator(last), d_first);
}

template <
    input_iterator InputIt, typename Size,
    nothrow_forward_iterator NoThrowForwardIt>
constexpr auto
    uninitialized_move_n(InputIt first, Size count, NoThrowForwardIt d_first)
        -> std::pair<InputIt, NoThrowForwardIt> {
    return uninitialized_copy_n(std::make_move_iterator(first), count, d_first);
}

template <nothrow_forward_iterator NoThrowForwardIt, typename T>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_fill(
    NoThrowForwardIt first, NoThrowForwardIt last,
    const T& value) noexcept(_raw_mem_algos::_n_fill<NoThrowForwardIt, T>)
    -> void {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; first != last; ++first) {
            std::construct_at(std::addressof(*first), value);
        }
        return;
    }
#endif
    std::uninitialized_fill(first, last, value);
}

template <nothrow_forward_iterator NoThrowForwardIt, typename Size, typename T>
ATOM_CONSTEXPR_SINCE_CXX20 auto uninitialized_fill_n(
    NoThrowForwardIt first, Size count,
    const T& value) noexcept(_raw_mem_algos::_n_fill<NoThrowForwardIt, T>)
    -> NoThrowForwardIt {
#if !NEUTROM_STL_CONSTEXPR_RAW_MEM_ALGOS
    if ATOM_CONST_EVALUATED {
        for (; count > 0; ++first, --count) {
            std::construct_at(std::addressof(*first), value);
        }
        return first;
    }
#endif
    return std::uninitialized_fill_n(first, count, value);
}

} // namespace neutron
