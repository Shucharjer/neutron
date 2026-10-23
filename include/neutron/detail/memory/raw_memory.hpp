// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#include <iterator>
#include <memory>
#include <utility>
#include <version>

// NOTE: The raw memory algorithms in stl is constexpr since c++26.

#if !defined(__cpp_lib_raw_memory_algorithms) ||                               \
    __cpp_lib_raw_memory_algorithms < 202411L
    #include "neutron/detail/macros.hpp"
    #if !defined(__cpp_if_consteval) || __cpp_if_consteval < 202106L
        #include <type_traits> // IWYU pragma: keep, mis-report
    #endif
    #include "neutron/detail/utility/completion_guard.hpp"
#endif

namespace neutron {

#if defined(__cpp_lib_raw_memory_algorithms) &&                                \
    __cpp_lib_raw_memory_algorithms >= 202411L

// NOLINTBEGIN

    // NOTE: Only the non-execution-policy overloads are exposed so that the
    // API matches the C++20/23 fallback, which cannot implement constexpr
    // execution-policy overloads.

    #define DECL_RAW_MEM_ALGO_FROM_STL(fn)                                     \
        template <                                                             \
            std::input_iterator InputIt,                                       \
            std::forward_iterator NoThrowForwardIt>                            \
        constexpr auto fn(                                                     \
            InputIt first, InputIt last, NoThrowForwardIt d_first)             \
            -> NoThrowForwardIt {                                              \
            return std::fn(first, last, d_first);                              \
        }

DECL_RAW_MEM_ALGO_FROM_STL(uninitialized_copy)
DECL_RAW_MEM_ALGO_FROM_STL(uninitialized_move)

    #undef DECL_RAW_MEM_ALGO_FROM_STL

    #define DECL_RAW_MEM_ALGO_N_FROM_STL(fn)                                   \
        template <                                                             \
            std::input_iterator InputIt, typename Size,                        \
            std::forward_iterator NoThrowForwardIt>                            \
        constexpr auto fn(InputIt first, Size size, NoThrowForwardIt d_first)  \
            -> NoThrowForwardIt {                                              \
            return std::fn(first, size, d_first);                              \
        }

DECL_RAW_MEM_ALGO_N_FROM_STL(uninitialized_copy_n)

    #undef DECL_RAW_MEM_ALGO_N_FROM_STL

    // `uninitialized_move_n` returns both the advanced source iterator and
    // the destination iterator.
    #define DECL_RAW_MEM_ALGO_N_PAIR_FROM_STL(fn)                              \
        template <                                                             \
            std::input_iterator InputIt, typename Size,                        \
            std::forward_iterator NoThrowForwardIt>                            \
        constexpr auto fn(InputIt first, Size size, NoThrowForwardIt d_first)  \
            -> std::pair<InputIt, NoThrowForwardIt> {                          \
            return std::fn(first, size, d_first);                              \
        }

DECL_RAW_MEM_ALGO_N_PAIR_FROM_STL(uninitialized_move_n)

    #undef DECL_RAW_MEM_ALGO_N_PAIR_FROM_STL

    #define DECL_RAW_MEM_ALGO_FILL_FROM_STL(fn)                                \
        template <                                                             \
            std::forward_iterator NoThrowForwardIt, typename T>                \
        constexpr auto fn(                                                     \
            NoThrowForwardIt first, NoThrowForwardIt last, const T& value)     \
            -> void {                                                          \
            std::fn(first, last, value);                                       \
        }

DECL_RAW_MEM_ALGO_FILL_FROM_STL(uninitialized_fill)

    #undef DECL_RAW_MEM_ALGO_FILL_FROM_STL

    #define DECL_RAW_MEM_ALGO_FILL_N_FROM_STL(fn)                              \
        template <                                                             \
            std::forward_iterator NoThrowForwardIt, typename Size,             \
            typename T>                                                        \
        constexpr auto fn(                                                     \
            NoThrowForwardIt first, Size size, const T& value)                 \
            -> NoThrowForwardIt {                                              \
            return std::fn(first, size, value);                                \
        }

DECL_RAW_MEM_ALGO_FILL_N_FROM_STL(uninitialized_fill_n)

    #undef DECL_RAW_MEM_ALGO_FILL_N_FROM_STL

    #define DECL_RAW_MEM_ALGO_CONSTRUCT_FROM_STL(fn)                           \
        template <std::forward_iterator NoThrowForwardIt>                      \
        constexpr auto fn(                                                     \
            NoThrowForwardIt first, NoThrowForwardIt last) -> void {           \
            std::fn(first, last);                                              \
        }

DECL_RAW_MEM_ALGO_CONSTRUCT_FROM_STL(uninitialized_default_construct)
DECL_RAW_MEM_ALGO_CONSTRUCT_FROM_STL(uninitialized_value_construct)

    #undef DECL_RAW_MEM_ALGO_CONSTRUCT_FROM_STL

    #define DECL_RAW_MEM_ALGO_CONSTRUCT_N_FROM_STL(fn)                         \
        template <                                                             \
            std::forward_iterator NoThrowForwardIt, typename Size>             \
        constexpr auto fn(NoThrowForwardIt first, Size size)                   \
            -> NoThrowForwardIt {                                              \
            return std::fn(first, size);                                       \
        }

DECL_RAW_MEM_ALGO_CONSTRUCT_N_FROM_STL(uninitialized_default_construct_n)
DECL_RAW_MEM_ALGO_CONSTRUCT_N_FROM_STL(uninitialized_value_construct_n)

    #undef DECL_RAW_MEM_ALGO_CONSTRUCT_N_FROM_STL

// NOLINTEND

#else

template <std::forward_iterator NoThrowForwardIt>
constexpr auto uninitialized_default_construct(
    NoThrowForwardIt first, NoThrowForwardIt last) -> void {
    if ATOM_CONST_EVALUATED {
        // NOTE: `construct_at` without arguments performs value-initialization.
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; dest != last; ++dest) {
            std::construct_at(std::addressof(*dest));
        }
        guard.dismiss();
        return;
    }

    std::uninitialized_default_construct(first, last);
}

template <std::forward_iterator NoThrowForwardIt, typename Size>
constexpr auto
    uninitialized_default_construct_n(NoThrowForwardIt first, Size count)
        -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; count > 0; ++dest, --count) {
            std::construct_at(std::addressof(*dest));
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_default_construct_n(first, count);
}

template <std::forward_iterator NoThrowForwardIt>
constexpr auto uninitialized_value_construct(
    NoThrowForwardIt first, NoThrowForwardIt last) -> void {
    if ATOM_CONST_EVALUATED {
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; dest != last; ++dest) {
            std::construct_at(std::addressof(*dest));
        }
        guard.dismiss();
        return;
    }

    std::uninitialized_value_construct(first, last);
}

template <std::forward_iterator NoThrowForwardIt, typename Size>
constexpr auto
    uninitialized_value_construct_n(NoThrowForwardIt first, Size count)
        -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; count > 0; ++dest, --count) {
            std::construct_at(std::addressof(*dest));
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_value_construct_n(first, count);
}

template <std::input_iterator InputIt, std::forward_iterator NoThrowForwardIt>
constexpr auto uninitialized_copy(
    InputIt first, InputIt last, NoThrowForwardIt d_first)
    -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = d_first;
        auto guard = make_exception_guard([d_first, &dest]() noexcept {
            std::destroy(d_first, dest);
        });
        for (; first != last; ++first, ++dest) {
            std::construct_at(std::addressof(*dest), *first);
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_copy(first, last, d_first);
}

template <
    std::input_iterator InputIt, typename Size,
    std::forward_iterator NoThrowForwardIt>
constexpr auto
    uninitialized_copy_n(InputIt first, Size count, NoThrowForwardIt d_first)
        -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = d_first;
        auto guard = make_exception_guard([d_first, &dest]() noexcept {
            std::destroy(d_first, dest);
        });
        for (; count > 0; ++first, ++dest, --count) {
            std::construct_at(std::addressof(*dest), *first);
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_copy_n(first, count, d_first);
}

template <std::input_iterator InputIt, std::forward_iterator NoThrowForwardIt>
constexpr auto uninitialized_move(
    InputIt first, InputIt last, NoThrowForwardIt d_first)
    -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = d_first;
        auto guard = make_exception_guard([d_first, &dest]() noexcept {
            std::destroy(d_first, dest);
        });
        for (; first != last; ++first, ++dest) {
            std::construct_at(std::addressof(*dest), std::move(*first));
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_move(first, last, d_first);
}

template <
    std::input_iterator InputIt, typename Size,
    std::forward_iterator NoThrowForwardIt>
constexpr auto
    uninitialized_move_n(InputIt first, Size count, NoThrowForwardIt d_first)
        -> std::pair<InputIt, NoThrowForwardIt> {
    if ATOM_CONST_EVALUATED {
        auto dest  = d_first;
        auto guard = make_exception_guard([d_first, &dest]() noexcept {
            std::destroy(d_first, dest);
        });
        for (; count > 0; ++first, ++dest, --count) {
            std::construct_at(std::addressof(*dest), std::move(*first));
        }
        guard.dismiss();
        return { first, dest };
    }

    return std::uninitialized_move_n(first, count, d_first);
}

template <std::forward_iterator NoThrowForwardIt, typename T>
constexpr auto uninitialized_fill(
    NoThrowForwardIt first, NoThrowForwardIt last, const T& value) -> void {
    if ATOM_CONST_EVALUATED {
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; dest != last; ++dest) {
            std::construct_at(std::addressof(*dest), value);
        }
        guard.dismiss();
        return;
    }

    std::uninitialized_fill(first, last, value);
}

template <std::forward_iterator NoThrowForwardIt, typename Size, typename T>
constexpr auto
    uninitialized_fill_n(NoThrowForwardIt first, Size count, const T& value)
        -> NoThrowForwardIt {
    if ATOM_CONST_EVALUATED {
        auto dest  = first;
        auto guard = make_exception_guard([first, &dest]() noexcept {
            std::destroy(first, dest);
        });
        for (; count > 0; ++dest, --count) {
            std::construct_at(std::addressof(*dest), value);
        }
        guard.dismiss();
        return dest;
    }

    return std::uninitialized_fill_n(first, count, value);
}

#endif

} // namespace neutron
