#pragma once
#include <algorithm>
#include <array>
#include <atomic>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)
    #include <xmmintrin.h>
#endif

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) ||            \
    defined(_M_X64)
inline void cpu_relax() { _mm_pause(); }
#elif defined(__aarch64__)
inline void cpu_relax() { __asm__ volatile("yield"); }
#else
inline void cpu_relax() {}
#endif

const auto max_spin_time = 1024;

} // namespace internal
/*! @endcond */

/**
 * @class spinlock
 * @brief Triditional spin lock, suitable for high-frequency task scenarios.
 * @details Normal spin lock, spin when calling `lock()`, which means low
 * latency.
 */
class spinlock;

} // namespace neutron

#if __has_include(<pthread.h>)
    #include <pthread.h>

namespace neutron {

class spinlock {
    pthread_spinlock_t lock_{};

public:
    // could not share with other processes.
    spinlock() noexcept { pthread_spin_init(&lock_, 0); }

    spinlock(const spinlock&)            = delete;
    spinlock& operator=(const spinlock&) = delete;
    spinlock(spinlock&& that) noexcept : lock_(std::exchange(that.lock_, 0)) {}
    spinlock& operator=(spinlock&& that) noexcept {
        std::swap(lock_, that.lock_);
        return *this;
    }

    ~spinlock() noexcept {
        if (lock_ != 0) {
            pthread_spin_destroy(&lock_);
            lock_ = 0;
        }
    }

    void lock() noexcept { pthread_spin_lock(&lock_); }
    void unlock() noexcept { pthread_spin_unlock(&lock_); }
    bool try_lock() noexcept { return pthread_spin_trylock(&lock_) == 0; }
};

#else

class spinlock {
public:
    spinlock()                           = default;
    spinlock(const spinlock&)            = delete;
    spinlock(spinlock&&)                 = delete;
    spinlock& operator=(const spinlock&) = delete;
    spinlock& operator=(spinlock&&)      = delete;
    ~spinlock()                          = default;

    /**
     * @brief Try get the lock.
     *
     */
    auto try_lock() noexcept -> bool {
        return flag_.test_and_set(std::memory_order_acquire);
    }

    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            internal::cpu_relax();
        }
    }

    void unlock() noexcept { flag_.clear(std::memory_order_release); }

private:
    #if defined(__cpp_lib_atomic_value_initialization) &&                      \
        __cpp_lib_atomic_value_initialization >= 201911L
    std::atomic_flag flag_;
    #else
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    #endif
};

#endif

#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L

/**
 * @class hybrid_spinlock
 * @brief A new implementation of spin lock inspired by atomic_wiat in c++20
 * @details It will first try triditional spinning and wait for notification if
 * it cannot lock for a long time, which means versatility.
 */
class hybrid_spinlock {
public:
    hybrid_spinlock()                                  = default;
    hybrid_spinlock(const hybrid_spinlock&)            = delete;
    hybrid_spinlock(hybrid_spinlock&&)                 = delete;
    hybrid_spinlock& operator=(const hybrid_spinlock&) = delete;
    hybrid_spinlock& operator=(hybrid_spinlock&&)      = delete;
    ~hybrid_spinlock()                                 = default;

    auto try_lock() noexcept -> bool {
        return flag_.test_and_set(std::memory_order_acquire);
    }

    void lock() noexcept {
        for (auto i = 0; i < internal::max_spin_time &&
                         flag_.test_and_set(std::memory_order_acquire);
             ++i) {
            internal::cpu_relax();
        }

        while (flag_.test_and_set(std::memory_order_acquire)) {
            flag_.wait(true, std::memory_order_relaxed);
        }
    }

    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
        flag_.notify_one();
    }

private:
    #if defined(__cpp_lib_atomic_value_initialization) &&                      \
        __cpp_lib_atomic_value_initialization >= 201911L
    std::atomic_flag flag_;
    #else
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    #endif
};

#else
}

    #include <mutex>

namespace neutron {

using hyhybrid_spinlock = std::mutex;

#endif

template <size_t Count, typename Mutex, template <typename> typename Lock>
class lock_keeper {
public:
    template <typename... Mutexes>
    requires(sizeof...(Mutexes) == Count)
    explicit lock_keeper(Mutexes&... mutexes) noexcept
        : locks_{ Lock(const_cast<Mutex&>(mutexes))... } {}

    lock_keeper(const lock_keeper&)            = delete;
    lock_keeper(lock_keeper&&)                 = delete;
    lock_keeper& operator=(const lock_keeper&) = delete;
    lock_keeper& operator=(lock_keeper&&)      = delete;

    ~lock_keeper() noexcept = default;

    void run_away() noexcept {
        std::ranges::for_each(locks_, [](Lock<Mutex>& lock) { lock.unlock(); });
    }

private:
    std::array<Lock<Mutex>, Count> locks_;
};
} // namespace neutron
