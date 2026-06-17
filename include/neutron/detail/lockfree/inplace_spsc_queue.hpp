// IWYU pragma: private, include <neutron/lockfree.hpp>
#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <thread>
#include <type_traits>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/inplace.hpp"
#include "neutron/detail/utility/packed_uint.hpp"

namespace neutron {

template <typename T, std::size_t Size>
class inplace_spsc_queue : private inplace_storage<T, Size> {
    using _base   = inplace_storage<T, Size>;
    using _size_t = packed_size_t<Size>;

public:
    using value_type = T;

    static_assert((Size & (Size - 1)) == 0, "Size should be single bit");

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) ==
               tail_.load(std::memory_order_acquire);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 bool push_back(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        static_assert(Size != 0);
        _size_t tail        = tail_.load(std::memory_order_relaxed);
        _size_t next        = (tail + 1) & (Size - 1);
        std::uint16_t count = 0;
        while (head_.load(std::memory_order_acquire) == next) {
            if (count++ < (std::numeric_limits<std::uint16_t>::max)()) {
                std::this_thread::yield();
            } else [[unlikely]] {
                return false;
            }
        } // busy loop
        std::construct_at(this->data() + tail, val);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    ATOM_CONSTEXPR_SINCE_CXX26 bool
        push_back(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>) {
        static_assert(Size != 0);
        _size_t tail        = tail_.load(std::memory_order_relaxed);
        _size_t next        = (tail + 1) & (Size - 1);
        std::uint16_t count = 0;
        while (head_.load(std::memory_order_acquire) == next) {
            if (count++ < (std::numeric_limits<std::uint16_t>::max)()) {
                std::this_thread::yield();
            } else [[unlikely]] {
                return false;
            }
        } // busy loop
        std::construct_at(this->data() + tail, std::move(val));
        tail_.store(next, std::memory_order_release);
        return true;
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 T& front() noexcept {
        _size_t head = head_.load(std::memory_order_relaxed);
        return *(this->data() + head);
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 const T& front() const noexcept {
        _size_t head = head_.load(std::memory_order_relaxed);
        return *(this->data() + head);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void pop_front() noexcept {
        assert(!empty());
        auto head = head_.load(std::memory_order_relaxed);
        std::destroy_at(this->data() + head);
        auto next = (head + 1) & (Size - 1);
        head_.store(next, std::memory_order_release);
    }

private:
    alignas(hdi_size) std::atomic<_size_t> head_ = 0;
    alignas(hdi_size) std::atomic<_size_t> tail_ = 0;
};

} // namespace neutron
