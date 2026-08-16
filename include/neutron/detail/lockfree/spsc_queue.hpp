#pragma once
#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/lockfree/intrusive_spsc_queue.hpp"
#include "neutron/detail/memory/object_pool.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/utility/assert.hpp"

namespace neutron {

template <typename T, typename Alloc = std::allocator<T>>
class spsc_queue {
public:
    using value_type     = T;
    using allocator_type = rebind_alloc_t<Alloc, T>;
    using _alloc_traits  = std::allocator_traits<allocator_type>;
    using size_type      = std::size_t;

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    spsc_queue(size_type cap, const Al& alloc = {})
        : alloc_(alloc), data_(_alloc_traits::allocate(alloc_, cap)),
          capacity_(cap) {}

    spsc_queue(const spsc_queue&)                     = delete;
    spsc_queue& operator=(const spsc_queue&) noexcept = delete;

    spsc_queue(spsc_queue&&) noexcept;
    spsc_queue& operator=(spsc_queue&&) noexcept;

    bool empty() { return true; }

    ~spsc_queue() noexcept(std::is_nothrow_destructible_v<T>) {
        auto head = head_.load(std::memory_order_relaxed);
        auto tail = tail_.load(std::memory_order_relaxed);
        if (tail >= head) {
            //
        } else {
            //
        }
        // _alloc_traits::deallocate(alloc_, data_);
    }

    template <typename V>
    bool push_back(V&& val) noexcept(std::is_nothrow_constructible_v<T, V>) {
        auto tail = tail_.load(std::memory_order_relaxed);
        auto next = (tail + 1) % capacity_;
        while (next == head_.load(std::memory_order_acquire)) {}
        // _alloc_traits::construct(alloc_, data_ + tail, std::forward<V>(val));
        tail_.store(next, std::memory_order_release);
        return true;
    }

    ATOM_NODISCARD T& front() noexcept {
        auto head = head_.load(std::memory_order_relaxed);
    }

    ATOM_NODISCARD const T& front() const noexcept {
        auto head = head_.load(std::memory_order_relaxed);
    }

    void pop_front() {
        NEUTRON_ASSERT(!empty());
        auto head = head_.load(std::memory_order_relaxed);
        auto next = (head + 1) % capacity_;
        while (tail_.load(std::memory_order_acquire) == next) {}
        _alloc_traits::destroy(alloc_, data_ + head);
        head_.store(next, std::memory_order_release);
    }

private:
    ATOM_NO_UNIQUE_ADDR allocator_type alloc_;
    T* data_;
    size_type capacity_;

    // using pool_t = constcapacity_pool<sizeof(T), 256, alignof(T)>;
    // pool_t* pools_;
    // pool_t* free_;
    // size_type capacity_ = 0;

    alignas(hdi_size) std::atomic<size_type> head_ = 0;
    alignas(hdi_size) std::atomic<size_type> tail_ = 0;
};

} // namespace neutron
