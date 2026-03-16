// IWYU pragma: private, include <neutron/execution_resources.hpp>
#pragma once
#include <cstdint>
#include <new>
#include <stdexcept>
#include <system_error>
#include <thread>
#include "neutron/detail/macros.hpp"
#include "neutron/execution.hpp" // IWYU pragma: keep

#if defined(_WIN32)
    #define NOMINMAX
    #include <Windows.h>

#elif defined(__linux) || defined(__linux__)
    #include <pthread.h>
    #include <unistd.h>
    #if ATOM_USES_NUMA && __has_include(<numa.h>)
        #include <numa.h>
    #endif

#elif false

#else
#endif

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _affinity_thread {

inline bool _set_affinity(uint32_t core, uint32_t numa) // NOLINT
{
#if defined(__APPLE__) || defined(__MACH__)
    return true;
#endif

    uint32_t concurrency = std::thread::hardware_concurrency();
    concurrency          = concurrency != 0 ? concurrency : 1;
    if (core >= concurrency) [[unlikely]] {
        throw std::runtime_error("try bind unexisted core");
    }

#if defined(_WIN32) // ignore numa policy
    HANDLE handle  = GetCurrentThread();
    DWORD_PTR mask = 1ULL << core;
    SetThreadAffinityMask(handle, mask);
#elif defined(__linux) || defined(__linux__) // complete numa support
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    pthread_t this_pthread = pthread_self();
    int err                = 0;
    err = pthread_setaffinity_np(this_pthread, sizeof(cpu_set_t), &cpuset);
    if (err != 0) [[unlikely]] {
        throw std::system_error(
            err, std::generic_category(), "failed to set thread affinity");
    }

    #if ATOM_USES_NUMA && __has_include(<numa.h>)
    if (numa_available() == -1) [[unlikely]] {
        throw std::runtime_error("numa not available");
    }
    if (numa >= static_cast<uint32_t>(numa_max_node())) [[unlikely]] {
        throw std::runtime_error("try bind numa greater than max node");
    }
    err = numa_run_on_node(static_cast<int>(numa));
    if (err != 0) [[unlikely]] {
        throw std::system_error(
            err, std::generic_category(), "failed to set numa affinity");
    }
    #endif

#else
    return false;
#endif
    return true;
}

inline bool
    _set_affinity(uint32_t core, uint32_t numa, std::nothrow_t) // NOLINT
{
#if defined(__APPLE__) || defined(__MACH__)
    return true;
#endif

    uint32_t concurrency = std::thread::hardware_concurrency();
    concurrency          = concurrency != 0 ? concurrency : 1;
    if (core >= concurrency) [[unlikely]] {
        return false;
    }

#if defined(_WIN32) // ignore numa policy
    HANDLE handle  = GetCurrentThread();
    DWORD_PTR mask = 1ULL << core;
    SetThreadAffinityMask(handle, mask);
#elif defined(__linux) || defined(__linux__) // complete numa support
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    pthread_t this_pthread = pthread_self();
    int err                = 0;
    err = pthread_setaffinity_np(this_pthread, sizeof(cpu_set_t), &cpuset);
    if (err != 0) [[unlikely]] {
        return false;
    }

    #if ATOM_USES_NUMA && __has_include(<numa.h>)
    if (numa_available() == -1) [[unlikely]] {
        return false;
    }
    if (numa >= static_cast<uint32_t>(numa_max_node())) [[unlikely]] {
        return false;
    }
    err = numa_run_on_node(static_cast<int>(numa));
    if (err != 0) [[unlikely]] {
        return false;
    }
    #endif
#else
    return false;
#endif
    return true;
}

} // namespace _affinity_thread
/*! @endcond */

/**
 * @brief Sets CPU core affinity (and optionally NUMA node affinity) for the
 * current thread.
 *
 * This function attempts to bind the calling thread to a specific logical CPU
 * core. On Linux systems with NUMA support enabled (via ATOM_USES_NUMA and
 * libnuma), it also binds the thread to the specified NUMA node.
 *
 * - On macOS (__APPLE__ or __MACH__), this function is a no-op and always
 * returns true.
 * - On Windows (_WIN32), only CPU affinity is set; the \p numa parameter is
 * ignored.
 * - On Linux, both CPU and NUMA affinities are applied if NUMA support is
 * available and enabled.
 *
 * @param core The logical CPU core index (zero-based) to bind the thread to.
 *             Must be less than std::thread::hardware_concurrency().
 * @param numa The NUMA node ID to bind the thread to (only used on Linux with
 * NUMA support).
 * @return \c true if affinity was successfully set.
 * @throws std::runtime_error if \p core is out of range or NUMA is requested
 * but unavailable.
 * @throws std::system_error if a system call (e.g., pthread_setaffinity_np or
 * numa_run_on_node) fails. The error code and message reflect the underlying OS
 * error.
 * @note This version uses exception-throwing semantics for error reporting.
 */
inline bool set_affinity(uint32_t core, uint32_t numa = 0) {
    return _affinity_thread::_set_affinity(core, numa);
}

/**
 * @brief Sets CPU core affinity (and optionally NUMA node affinity) for the
 * current thread, using non-throwing semantics.
 *
 * Behaves identically to the throwing version of _set_affinity(), but instead
 * of throwing exceptions on error, it returns \c false.
 *
 * - On macOS, always returns \c true (no-op).
 * - On Windows, sets CPU affinity only; ignores \p numa.
 * - On Linux, applies both CPU and NUMA affinity if NUMA is enabled and
 * available.
 *
 * @param core The logical CPU core index (zero-based). Must be < hardware
 * concurrency.
 * @param numa The NUMA node ID (Linux-only, when NUMA support is enabled).
 * @param std::nothrow_t A tag type to select the non-throwing overload.
 * @return \c true on success, \c false if:
 *         - \p core is invalid,
 *         - system calls fail,
 *         - NUMA is requested but not available (on Linux),
 *         - or the platform does not support affinity control.
 * @note This overload never throws exceptions.
 */
inline bool
    set_affinity(uint32_t core, uint32_t numa, std::nothrow_t) noexcept {
    return _affinity_thread::_set_affinity(core, numa, std::nothrow);
}

class affinity_thread {
    class _affinity {
    public:
        ATOM_NODISCARD uint32_t get_core() const noexcept { return core_; }

#if defined(__linux) || defined(__linux__)
        constexpr _affinity(uint32_t core, uint32_t numa) noexcept // NOLINT
            : core_(core), numa_(numa) {}

        ATOM_NODISCARD uint32_t get_numa() const noexcept { return numa_; }
#else
        constexpr _affinity(uint32_t core, uint32_t numa) noexcept // NOLINT
            : core_(core) {}

        ATOM_NODISCARD uint32_t get_numa() const noexcept { return 0; }
#endif

    private:
        uint32_t core_;
#if defined(__linux) || defined(__linux__)
        uint32_t numa_;
#endif
    };

public:
    explicit affinity_thread(uint32_t core, uint32_t numa = 0)
        : affinity_{ core, numa }, loop_(), thread_([this] {
              consistancy_ =
                  set_affinity(affinity_.get_core(), affinity_.get_numa());
              loop_.run();
          }) {}

    affinity_thread(uint32_t core, uint32_t numa, std::nothrow_t)
        : affinity_{ core, numa }, loop_(), thread_([this] {
              consistancy_ = set_affinity(
                  affinity_.get_core(), affinity_.get_numa(), std::nothrow);
              loop_.run();
          }) {}

    affinity_thread(const affinity_thread&)            = delete;
    affinity_thread& operator=(const affinity_thread&) = delete;
    affinity_thread(affinity_thread&&)                 = delete;
    affinity_thread& operator=(affinity_thread&&)      = delete;

    ~affinity_thread() {
        loop_.finish();
        thread_.join();
    }

    auto get_scheduler() noexcept { return loop_.get_scheduler(); }

    ATOM_NODISCARD uint32_t core_id() const noexcept {
        return affinity_.get_core();
    }

    ATOM_NODISCARD uint32_t numa_id() const noexcept {
        return affinity_.get_numa();
    }

    ATOM_NODISCARD bool consistancy() const noexcept { return consistancy_; }

    ATOM_NODISCARD
    auto get_id() const noexcept -> std::thread::id { return thread_.get_id(); }

private:
    bool consistancy_{};
    _affinity affinity_;
    execution::run_loop loop_;
    std::thread thread_;
};

} // namespace neutron
