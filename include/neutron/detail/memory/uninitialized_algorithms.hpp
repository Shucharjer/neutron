#pragma once
#include <version>
#if defined(__cpp_lib_raw_memory_algorithms) &&                                \
    __cpp_lib_raw_memory_algorithms >= 202411L

namespace neutron {

using std::uninitialized_default_construct;
using std::uninitialized_default_construct_n;
using std::uninitialized_value_construct;
using std::uninitialized_value_construct_n;
using std::uninitialized_copy;
using std::uninitialized_copy_n;
using std::uninitialized_move;
using std::uninitialized_move_n;
using std::uninitialized_fill;
using std::uninitialized_fill_n;

} // namespace neutron

#else

    #include <algorithm>
    #include <iterator>
    #include <memory>
    #include "neutron/detail/macros.hpp"

namespace neutron {

template <typename InputIterator, typename ForwardIterator>
constexpr ForwardIterator uninitialized_copy(
    InputIterator first, InputIterator last, ForwardIterator result) {
    using value_type = std::iter_value_t<ForwardIterator>;

    if ATOM_CONST_EVALUATED {
        return std::copy(first, last, result);
    }
    return std::uninitialized_copy(first, last, result);
}

template <typename InputIterator, typename Size, typename ForwardIterator>
constexpr ForwardIterator
    uninitialized_copy_n(InputIterator first, Size n, ForwardIterator result) {

    if ATOM_CONST_EVALUATED {
        for (Size i = 0; i < n; ++i, ++first) {
            result = *first;
        }
        return result;
    }
    return std::uninitialized_copy_n(first, n, result);
}

template <typename InputIterator, typename ForwardIterator>
constexpr ForwardIterator uninitialized_move(
    InputIterator first, InputIterator last, ForwardIterator result) {
    using value_type = std::iter_value_t<ForwardIterator>;

    if ATOM_CONST_EVALUATED {
        return std::move(first, last, result);
    }
    return std::uninitialized_move(first, last, result);
}

template <typename InputIterator, typename Size, typename ForwardIterator>
constexpr ForwardIterator
    uninitialized_move_n(InputIterator first, Size n, ForwardIterator result) {

    if ATOM_CONST_EVALUATED {
        for (Size i = 0; i < n; ++i, ++first) {
            result = *first;
        }
        return result;
    }
    return std::uninitialized_move_n(first, n, result);
}

template <typename InputIterator, typename ForwardIterator>
constexpr ForwardIterator uninitialized_fill(
    InputIterator first, InputIterator last, ForwardIterator result) {
    using value_type = std::iter_value_t<ForwardIterator>;

    if ATOM_CONST_EVALUATED {
        return std::fill(first, last, result);
    }
    return std::uninitialized_fill(first, last, result);
}

template <typename InputIterator, typename Size, typename ForwardIterator>
constexpr ForwardIterator
    uninitialized_fill_n(InputIterator first, Size n, ForwardIterator result) {
    using value_type = std::iter_value_t<ForwardIterator>;

    if ATOM_CONST_EVALUATED {
        return std::fill_n(first, n, result);
    }
    return std::uninitialized_fill_n(first, n, result);
}

} // namespace neutron

#endif
