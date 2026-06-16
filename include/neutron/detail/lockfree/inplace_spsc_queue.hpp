// IWYU pragma: private, include <neutron/lockfree.hpp>
#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
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

    static constexpr bool eligible_for_copy =
        std::is_copy_constructible_v<T> && sizeof(T) <= (sizeof(void*) << 1);

public:
    static_assert((Size & (Size - 1)) == 0, "Size should be single bit");

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) ==
               tail_.load(std::memory_order_relaxed);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void
        push_back(T val) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires eligible_for_copy
    {
        static_assert(Size != 0);
        _size_t tail = tail_.load(std::memory_order_relaxed);
        _size_t next = (tail + 1) & (Size - 1);
        while (head_.load(std::memory_order_acquire) == next) {} // busy loop
        std::construct_at(this->data() + tail, val);
        tail_.store(next, std::memory_order_release);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void push_back(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
    requires(!eligible_for_copy)
    {
        static_assert(Size != 0);
        _size_t tail = tail_.load(std::memory_order_relaxed);
        _size_t next = (tail + 1) & (Size - 1);
        while (head_.load(std::memory_order_acquire) == next) {} // busy loop
        std::construct_at(this->data() + tail, val);
        tail_.store(next, std::memory_order_release);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void
        push_back(T&& val) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(!eligible_for_copy)
    {
        static_assert(Size != 0);
        _size_t tail = tail_.load(std::memory_order_relaxed);
        _size_t next = (tail + 1) & (Size - 1);
        while (head_.load(std::memory_order_acquire) == next) {} // busy loop
        std::construct_at(this->data() + tail, std::move(val));
        tail_.store(next, std::memory_order_release);
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
        auto next = (head + 1) & (Size - 1);
        while (tail_.load(std::memory_order_acquire) == next) {}
        std::destroy_at(this->data() + head);
        head_.store(next, std::memory_order_release);
    }

private:
    alignas(hdi_size) std::atomic<_size_t> head_ = 0;
    alignas(hdi_size) std::atomic<_size_t> tail_ = 0;
};

} // namespace neutron
