#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <atomic>
    #include <cstddef>
#endif
#include "neutron/detail/parallel/cpu_relax.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <std::size_t SpinCount = 8192>
class spin_mutex {
public:
    spin_mutex() noexcept                    = default;
    spin_mutex(const spin_mutex&)            = delete;
    spin_mutex(spin_mutex&&)                 = delete;
    spin_mutex& operator=(const spin_mutex&) = delete;
    spin_mutex& operator=(spin_mutex&&)      = delete;
    ~spin_mutex() noexcept                   = default;

    auto try_lock() noexcept -> bool {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void lock() noexcept {
        for (std::size_t i = 0;
             i < SpinCount && flag_.test(std::memory_order_acquire); ++i) {
            cpu_relax();
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
#if defined(__cpp_lib_atomic_value_initialization) &&                          \
    __cpp_lib_atomic_value_initialization >= 201911L
    std::atomic_flag flag_;
#else
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
#endif
};

} // namespace neutron
