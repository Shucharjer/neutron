// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <cerrno>
#include <cstdlib>
#include <system_error>
#include "neutron/detail/utility/macros.hpp"

#if defined(_WIN32)
// Matches the declaration in <errhandlingapi.h>, so that <windows.h> is not
// dragged into every user of <neutron/utility.hpp>.
extern "C" __declspec(dllimport) unsigned long __stdcall GetLastError(void);
#endif

namespace neutron {

/**
 * @brief Error code of the last failed system call on the calling thread.
 *
 * `GetLastError()` on Windows, `errno` elsewhere.
 */
ATOM_NODISCARD inline int last_system_error() noexcept {
#if defined(_WIN32)
    return static_cast<int>(::GetLastError());
#else
    return errno;
#endif
}

/**
 * @brief Throw `std::system_error` in the system category.
 *
 * Calls `std::abort()` instead when exceptions are disabled.
 */
[[noreturn]] inline void throw_system_error(int code, const char* what) {
#if ATOM_ENABLE_EXCEPTIONS
    throw std::system_error(code, std::system_category(), what);
#else
    (void)code;
    (void)what;
    std::abort();
#endif
}

/**
 * @brief Throw `std::system_error` for the last failed system call.
 */
[[noreturn]] inline void throw_last_system_error(const char* what) {
    throw_system_error(last_system_error(), what);
}

} // namespace neutron
