#pragma once
#include <atomic>
#include <cstdio>
#include <source_location>

/**
 * @file
 * @brief Minimal assertion helpers for the test executables.
 *
 * A failed check prints the expression and its location and is counted, but
 * does not stop the test, so one run reports every failure. Each test ends
 * with `return neutron_test::result();`, which makes ctest see the failure.
 *
 * ```cpp
 * void test_push() {
 *     queue.push_back(1);
 *     require(queue.size() == 1);
 * }
 *
 * int main() {
 *     test_push();
 *     return neutron_test::result();
 * }
 * ```
 */

namespace neutron_test {

inline std::atomic<int> failures = 0;

inline bool check(
    bool passed, bool expected, const char* expression,
    const std::source_location& loc) noexcept {
    if (passed != expected) {
        failures.fetch_add(1, std::memory_order_relaxed);
        std::fprintf(
            stderr, "%s:%u: in %s\n  %s(%s) failed\n", loc.file_name(),
            static_cast<unsigned>(loc.line()), loc.function_name(),
            expected ? "require" : "require_false", expression);
    }
    return passed == expected;
}

/// @brief Exit code of the test: 0 when every check passed.
inline int result() noexcept {
    return failures.load(std::memory_order_relaxed) == 0 ? 0 : 1;
}

} // namespace neutron_test

// Variadic, so template arguments with commas need no extra parentheses.

/// @brief Checks that the expression is true.
#define require(...)                                                           \
    ::neutron_test::check(                                                     \
        static_cast<bool>(__VA_ARGS__), true, #__VA_ARGS__,                    \
        std::source_location::current())

/// @brief Checks that the expression is false.
#define require_false(...)                                                     \
    ::neutron_test::check(                                                     \
        static_cast<bool>(__VA_ARGS__), false, #__VA_ARGS__,                   \
        std::source_location::current())

/// @brief Checks the expression; returns `ret` from the caller on failure,
/// for checks the rest of the function depends on.
#define require_or_return(expr, ret)                                           \
    do {                                                                       \
        if (!require(expr)) {                                                  \
            return ret;                                                        \
        }                                                                      \
    } while (false)

#define require_false_or_return(expr, ret)                                     \
    do {                                                                       \
        if (!require_false(expr)) {                                            \
            return ret;                                                        \
        }                                                                      \
    } while (false)
