// IWYU pragma: private, include <neutron/lockfree.hpp>
#pragma once
#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/memory/inplace.hpp"
#include "neutron/detail/parallel/cpu_relax.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

template <typename T, std::size_t Size>
requires(std::has_single_bit(Size) && std::is_nothrow_destructible_v<T>)
class inplace_spsc_queue;

class _inplace_spsc_queue_base {
    template <typename T, std::size_t Size>
    requires(std::has_single_bit(Size) && std::is_nothrow_destructible_v<T>)
    friend class inplace_spsc_queue;

    using size_type = std::size_t;

    /// @brief Producer-owned state.
    alignas(hdi_size) std::atomic<size_type> tail_{ 0 };
    size_type head_cache_{ 0 }; ///< A stale copy of `head_`.

    /// @brief Consumer-owned state.
    alignas(hdi_size) std::atomic<size_type> head_{ 0 };
    mutable size_type tail_cache_{ 0 }; ///< A stale copy of `tail_`.

    /// @brief Keeps the slots off the consumer's line, as the Itanium ABI
    /// places later bases in the tail padding of this class.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    std::byte padding_[hdi_size - sizeof(head_) - sizeof(tail_cache_)]{};
};

/**
 * @brief A bounded Single-Producer Single-Consumer (SPSC) lock-free queue with
 * inline storage.
 *
 * Exactly one thread may call `push_back`, and exactly one
 * (other) thread may call `empty`, `front` and `pop_front`. All `Size` slots
 * are usable.
 *
 * Both ends advance free-running counters and keep a cached copy of the
 * opposite counter, so the shared counters are only read when the cache
 * claims the queue to be full (producer) or empty (consumer).
 *
 * @tparam T The type of the stored values.
 * @tparam Size The capacity, which must be a power of two.
 */
template <typename T, std::size_t Size>
requires(std::has_single_bit(Size) && std::is_nothrow_destructible_v<T>)
class inplace_spsc_queue :
    private _inplace_spsc_queue_base,
    private inplace_storage<T, Size> {
public:
    using value_type = T;
    using size_type  = std::size_t;

    // Slots are constructed on push.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    inplace_spsc_queue() noexcept = default;

    inplace_spsc_queue(const inplace_spsc_queue&)            = delete;
    inplace_spsc_queue& operator=(const inplace_spsc_queue&) = delete;
    inplace_spsc_queue(inplace_spsc_queue&&)                 = delete;
    inplace_spsc_queue& operator=(inplace_spsc_queue&&)      = delete;

    ATOM_CONSTEXPR_SINCE_CXX26 ~inplace_spsc_queue() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            const size_type tail = tail_.load(std::memory_order_relaxed);
            for (size_type head = head_.load(std::memory_order_relaxed);
                 head != tail; ++head) {
                std::destroy_at(_slot(head));
            }
        }
    }

    /**
     * @brief Checks whether the queue is empty.
     *
     * Only the consumer thread may call this function.
     */
    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 bool empty() const noexcept {
        const size_type head = head_.load(std::memory_order_relaxed);
        if (head != tail_cache_) [[likely]] {
            return false;
        }
        tail_cache_ = tail_.load(std::memory_order_acquire);
        return head == tail_cache_;
    }

    /**
     * @brief Constructs a value at the back of the queue, waiting for a free
     * slot while the queue is full.
     *
     * Only the producer thread may call this function.
     */
    template <typename... Args>
    requires std::constructible_from<T, Args...>
    ATOM_CONSTEXPR_SINCE_CXX26 void push_back(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        const size_type tail = tail_.load(std::memory_order_relaxed);
        if (tail - head_cache_ == Size) [[unlikely]] {
            _wait_for_slot(tail);
        }
        _emplace(tail, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns the value at the front of the queue.
     *
     * Only the consumer thread may call this function, and the queue must not
     * be empty.
     */
    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 T& front() noexcept {
        return *_slot(head_.load(std::memory_order_relaxed));
    }

    /// @copydoc front()
    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 const T& front() const noexcept {
        return *_slot(head_.load(std::memory_order_relaxed));
    }

    /**
     * @brief Destroys the front value.
     *
     * Only the consumer thread may call this function, and the queue must not
     * be empty.
     */
    ATOM_CONSTEXPR_SINCE_CXX26 void pop_front() noexcept {
        const size_type head = head_.load(std::memory_order_relaxed);
        std::destroy_at(_slot(head));
        head_.store(head + 1, std::memory_order_release);
    }

private:
    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 T*
        _slot(size_type index) noexcept {
        return this->data() + (index & (Size - 1));
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 const T*
        _slot(size_type index) const noexcept {
        return this->data() + (index & (Size - 1));
    }

    template <typename... Args>
    ATOM_CONSTEXPR_SINCE_CXX26 void _emplace(size_type tail, Args&&... args) {
        std::construct_at(_slot(tail), std::forward<Args>(args)...);
        tail_.store(tail + 1, std::memory_order_release);
    }

    ATOM_NOINLINE void _wait_for_slot(size_type tail) noexcept {
        std::remove_const_t<decltype(max_spin_time)> spin = 0;
        while (true) {
            head_cache_ = head_.load(std::memory_order_acquire);
            if (tail - head_cache_ != Size) {
                return;
            }
            if (spin != max_spin_time) {
                ++spin;
                cpu_relax();
            } else {
                std::this_thread::yield();
            }
        }
    }
};

} // namespace neutron
