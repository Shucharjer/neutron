#pragma once
// With modules, the includer imports std.
#ifndef ATOM_USES_MODULES
    #include <atomic>
    #include <iostream>
    #include <source_location>
#endif

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
        // `std::cerr` instead of `stderr`, which is a macro that modules do
        // not export.
        std::cerr << loc.file_name() << ':' << loc.line() << ": in "
                  << loc.function_name() << "\n  "
                  << (expected ? "require" : "require_false") << '('
                  << expression << ") failed\n";
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
