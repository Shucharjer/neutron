#pragma once
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/inplace.hpp"
#include "neutron/detail/utility/assert.hpp"
#include "neutron/detail/utility/packed_uint.hpp"

namespace neutron {

template <typename T, std::size_t Size>
class inplace_mpsc_queue : private inplace_storage<T, Size> {
    using _base = inplace_storage<T, Size>;

public:
    static_assert((Size & (Size - 1)) == 0, "Size should be single bit");

    inplace_mpsc_queue() noexcept {
        for (std::size_t index = 0; index != Size; ++index) {
            sequence_[index].store(index, std::memory_order_relaxed);
        }
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 bool empty() const noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        return sequence_[head & (Size - 1)].load(std::memory_order_acquire) !=
               head + 1;
    }

    template <typename Val>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        push_back(Val&& val) noexcept(std::is_nothrow_constructible_v<T, Val>) {
        static_assert(Size != 0);

        const auto pos = enqueue_pos_.fetch_add(1, std::memory_order_relaxed);
        auto& sequence = sequence_[pos & (Size - 1)];
        while (sequence.load(std::memory_order_acquire) != pos) {
            std::this_thread::yield();
        }
        std::construct_at(
            this->data() + (pos & (Size - 1)), std::forward<Val>(val));
        sequence.store(pos + 1, std::memory_order_release);
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 T& front() noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        return *(this->data() + (head & (Size - 1)));
    }

    ATOM_NODISCARD ATOM_CONSTEXPR_SINCE_CXX26 const T& front() const noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        return *(this->data() + (head & (Size - 1)));
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void pop_front() noexcept {
        NEUTRON_ASSERT(!empty());
        const auto head = head_.load(std::memory_order_relaxed);
        std::destroy_at(this->data() + (head & (Size - 1)));
        sequence_[head & (Size - 1)].store(
            head + Size, std::memory_order_release);
        head_.store(head + 1, std::memory_order_relaxed);
    }

private:
    alignas(hdi_size) std::atomic<std::size_t> head_        = 0;
    alignas(hdi_size) std::atomic<std::size_t> enqueue_pos_ = 0;
    std::array<std::atomic<std::size_t>, Size> sequence_{};
};

} // namespace neutron
