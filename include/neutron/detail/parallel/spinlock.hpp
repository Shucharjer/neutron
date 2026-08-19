// IWYU pragma: private, include <neutron/parallel.hpp>
#pragma once
#include <atomic>
#include "neutron/detail/parallel/cpu_relax.hpp"

namespace neutron {

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
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            cpu_relax();
        }
    }

    void unlock() noexcept { flag_.clear(std::memory_order_release); }

private:
#if defined(__cpp_lib_atomic_value_initialization) &&                          \
    __cpp_lib_atomic_value_initialization >= 201911L
    std::atomic_flag flag_;
#else
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
#endif
};

} // namespace neutron
