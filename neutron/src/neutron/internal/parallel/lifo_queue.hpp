/**
 * @brief This file is the internal implementation of `lifo_queue`, which is
 * useful when build thread pool.
 * It references lifo_queue in exec.
 * We use the LIFO queue because newly submitted tasks are often still in the
 * cache, which allows us to complete a task quickly. This is unfair to tasks
 * submitted earlier, so we steal from the head of the queue.
 *
 * @date 2025-11-23
 */

#pragma once
#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>
#include "../allocator.hpp"
#include "../macros.hpp"
#include "./cpu_relax.hpp"

namespace neutron {

enum class lifo_queue_error_code : uint_fast8_t {
    success,
    done,
    empty,
    full,
    conflict
};

template <class Ty>
struct fetch_result {
    lifo_queue_error_code status;
    Ty value;
};

struct takeover_result {
    size_t front;
    size_t back;
};

template <typename Ty, _std_simple_allocator Alloc = std::allocator<Ty>>
class lifo_queue {
    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

public:
    using value_type      = Ty;
    using allocator_type  = _allocator_t<Ty>;
    using alloc_traits    = std::allocator_traits<allocator_type>;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;
    using reference       = Ty&;
    using const_reference = const Ty&;
    using size_type       = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;

    template <typename Al = Alloc>
    CONSTEXPR26 explicit lifo_queue(
        size_type num_blocks, size_type block_size, const Al& alloc = {});

    CONSTEXPR26 auto pop_back() noexcept -> Ty;

    CONSTEXPR26 auto push_back(Ty value) noexcept -> bool;

    CONSTEXPR26 auto steal_front() noexcept -> Ty;

    template <std::forward_iterator Iter, std::sentinel_for<Iter> Sentinel>
    CONSTEXPR26 auto push_back(Iter first, Sentinel last) noexcept -> Iter;

    NODISCARD constexpr auto get_available_capacity() const noexcept
        -> size_type;

    NODISCARD CONSTEXPR26 auto get_free_capacity() const noexcept -> size_type;

    NODISCARD constexpr auto block_size() const noexcept -> size_type;

    NODISCARD constexpr auto num_blocks() const noexcept -> size_type;

private:
    class block_t {
    public:
        template <typename Al>
        CONSTEXPR26 explicit block_t(size_t block_size, const Al& alloc = {})
            : head_{ 0 }, tail_{ 0 }, steal_head_{ 0 },
              steal_tail_{ block_size },
              ring_buffer_(block_size, allocator_type{ alloc }) {}

        CONSTEXPR26 block_t(const block_t& that)
            : ring_buffer_(that.ring_buffer_) {
            head_.store(
                that.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                that.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                that.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                that.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }

        CONSTEXPR26 block_t& operator=(const block_t& that) {
            head_.store(
                that.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                that.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                that.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                that.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            ring_buffer_ = that.ring_buffer_;
            return *this;
        }

        CONSTEXPR26 block_t(block_t&& that) noexcept {
            head_.store(
                that.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                that.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                that.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                that.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            // NOLINTNEXTLINE
            ring_buffer_ = std::exchange(std::move(that.ring_buffer_), {});
        }

        CONSTEXPR26 block_t& operator=(block_t&& other) noexcept {
            head_.store(
                other.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                other.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                other.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                other.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            ring_buffer_ = std::exchange(std::move(other.ring_buffer_), {});
            return *this;
        }

        CONSTEXPR26 ~block_t() noexcept = default;

        CONSTEXPR26 auto put(Ty value) noexcept -> lifo_queue_error_code {
            using enum lifo_queue_error_code;

            uint64_t back = tail_.load(std::memory_order_relaxed);
            if (back < size()) [[likely]] {
                ring_buffer_[static_cast<size_type>(back)] = std::move(value);
                tail_.store(back + 1, std::memory_order_release);
                return success;
            }
            return full;
        }

        template <std::forward_iterator Iter, std::sentinel_for<Iter> Sentinel>
        CONSTEXPR26 auto bulk_put(Iter first, Sentinel last) noexcept -> Iter {
            uint64_t back = tail_.load(std::memory_order_relaxed);
            while (first != last && back < size()) {
                ring_buffer_[static_cast<size_type>(back)] = std::move(*first);
                ++back;
                ++first;
            }
            tail_.store(back, std::memory_order_release);
            return first;
        }

        CONSTEXPR26 auto get() noexcept -> fetch_result<Ty> {
            using enum lifo_queue_error_code;

            uint64_t front = head_.load(std::memory_order_relaxed);
            if (front == size()) [[unlikely]] {
                return { done, {} };
            }
            uint64_t back = tail_.load(std::memory_order_relaxed);
            if (front == back) [[unlikely]] {
                return { empty, {} };
            }
            fetch_result<Ty> result{
                success,
                std::move(ring_buffer_[static_cast<size_type>(back - 1)])
            };
            tail_.store(back - 1, std::memory_order_release);
            return result;
        }

        CONSTEXPR26 auto steal() noexcept -> fetch_result<Ty> {
            using enum lifo_queue_error_code;

            uint64_t spos = steal_tail_.load(std::memory_order_relaxed);
            if (spos == size()) [[unlikely]] {
                return { done, {} };
            }
            uint64_t back = tail_.load(std::memory_order_acquire);
            if (spos == back) [[unlikely]] {
                return { empty, {} };
            }
            if (!steal_tail_.compare_exchange_strong(
                    spos, spos + 1, std::memory_order_relaxed)) {
                return { conflict, {} };
            }
            fetch_result<Ty> result{
                success, std::move(ring_buffer_[static_cast<size_type>(spos)])
            };
            steal_head_.fetch_add(1, std::memory_order_release);
            return result;
        }

        CONSTEXPR26 auto takeover() noexcept -> takeover_result {
            uint64_t spos =
                steal_tail_.exchange(size(), std::memory_order_relaxed);
            if (spos == size()) [[unlikely]] {
                return { .front = static_cast<size_type>(
                             head_.load(std::memory_order_relaxed)),
                         .back = static_cast<size_type>(
                             tail_.load(std::memory_order_relaxed)) };
            }
            head_.store(spos, std::memory_order_relaxed);
            return { .front = static_cast<size_type>(spos),
                     .back  = static_cast<size_type>(
                         tail_.load(std::memory_order_relaxed)) };
        }

        NODISCARD CONSTEXPR26 bool is_writable() const noexcept {
            uint64_t expected_steal = size();
            uint64_t spos = steal_tail_.load(std::memory_order_relaxed);
            return spos == expected_steal;
        }

        NODISCARD CONSTEXPR26 size_type free_capacity() const noexcept {
            uint64_t back = tail_.load(std::memory_order_relaxed);
            return size() - back;
        }

        CONSTEXPR26 void grant() noexcept {
            uint64_t old_head =
                head_.exchange(size(), std::memory_order_relaxed);
            steal_tail_.store(old_head, std::memory_order_release);
        }

        CONSTEXPR26 bool reclaim() noexcept {
            uint64_t expected_shead = tail_.load(std::memory_order_relaxed);
            while (steal_head_.load(std::memory_order_acquire) !=
                   expected_shead) {
                internal::cpu_relax();
            }
            head_.store(0, std::memory_order_relaxed);
            tail_.store(0, std::memory_order_relaxed);
            steal_tail_.store(size(), std::memory_order_relaxed);
            steal_head_.store(0, std::memory_order_relaxed);
            return false;
        }

        NODISCARD CONSTEXPR26 bool is_stealable() const noexcept {
            return steal_tail_.load(std::memory_order_acquire) != size();
        }

        NODISCARD constexpr size_type size() const noexcept {
            return ring_buffer_.size();
        }

    private:
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint64_t> head_;
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint64_t> tail_;
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint64_t> steal_head_;
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint64_t> steal_tail_;
        std::vector<Ty, _allocator_t<Ty>> ring_buffer_;
    };

    CONSTEXPR26 bool _advance_get_index() noexcept;
    CONSTEXPR26 bool
        _advance_steal_index(size_t expected_thief_counter) noexcept;
    CONSTEXPR26 bool _advance_put_index() noexcept;

    alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> owner_block_{ 1 };
    alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> thief_block_{ 0 };
    std::vector<block_t, _allocator_t<block_t>> blocks_;
    size_t mask_;
};

template <typename Ty, _std_simple_allocator Alloc>
template <typename Al>
CONSTEXPR26 lifo_queue<Ty, Alloc>::lifo_queue(
    size_type num_blocks, size_type block_size, const Al& alloc)
    : blocks_(
          std::max(static_cast<size_type>(2), std::bit_ceil(num_blocks)),
          block_t(block_size, alloc), _allocator_t<block_t>(alloc)),
      mask_(blocks_.size() - 1) {
    blocks_[owner_block_.load()].reclaim();
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::pop_back() noexcept -> Ty {
    using enum lifo_queue_error_code;

    do {
        size_t owner_index =
            owner_block_.load(std::memory_order_relaxed) & mask_;
        block_t& current_block = blocks_[owner_index];
        auto [ec, value]       = current_block.get();
        if (ec == success) {
            return value;
        }
        if (ec == done) {
            return {};
        }
    } while (_advance_get_index());
    return {};
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::steal_front() noexcept -> Ty {
    using enum lifo_queue_error_code;

    size_t thief = 0;
    do {
        thief               = thief_block_.load(std::memory_order_relaxed);
        size_t thief_index  = thief & mask_;
        block_t& block      = blocks_[thief_index];
        fetch_result result = block.steal();
        while (result.status != done) {
            if (result.status == success) {
                return result.value;
            }
            if (result.status == empty) {
                return {};
            }
            result = block.steal();
        }
    } while (_advance_steal_index(thief));
    return {};
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::push_back(Ty value) noexcept -> bool {
    do {
        size_t owner_index =
            owner_block_.load(std::memory_order_relaxed) & mask_;
        block_t& block = blocks_[owner_index];
        auto status    = block.put(value);
        if (status == lifo_queue_error_code::success) {
            return true;
        }
    } while (_advance_put_index());
    return false;
}

template <typename Ty, _std_simple_allocator Alloc>
template <std::forward_iterator Iter, std::sentinel_for<Iter> Sentinel>
CONSTEXPR26 auto
    lifo_queue<Ty, Alloc>::push_back(Iter first, Sentinel last) noexcept
    -> Iter {
    do {
        size_t owner_index =
            owner_block_.load(std::memory_order_relaxed) & mask_;
        block_t& block = blocks_[owner_index];
        first          = block.bulk_put(first, last);
    } while (first != last && _advance_put_index());
    return first;
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::get_free_capacity() const noexcept
    -> size_type {
    size_t owner_counter  = owner_block_.load(std::memory_order_relaxed);
    size_t owner_index    = owner_counter & mask_;
    size_t local_capacity = blocks_[owner_index].free_capacity();
    size_t thief_counter  = thief_block_.load(std::memory_order_relaxed);
    size_t dist           = owner_counter - thief_counter;
    size_t rest           = blocks_.size() - dist - 1;
    return local_capacity + (rest * block_size());
}

template <typename Ty, _std_simple_allocator Alloc>
constexpr auto lifo_queue<Ty, Alloc>::get_available_capacity() const noexcept
    -> size_type {
    return num_blocks() * block_size();
}

template <typename Ty, _std_simple_allocator Alloc>
constexpr auto lifo_queue<Ty, Alloc>::block_size() const noexcept -> size_type {
    return blocks_[0].block_size();
}

template <typename Ty, _std_simple_allocator Alloc>
constexpr auto lifo_queue<Ty, Alloc>::num_blocks() const noexcept -> size_type {
    return blocks_.size();
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::_advance_get_index() noexcept -> bool {
    size_t owner_counter     = owner_block_.load(std::memory_order_relaxed);
    size_t predecessor       = owner_counter - 1UL;
    size_t predecessor_index = predecessor & mask_;
    block_t& previous_block  = blocks_[predecessor_index];
    takeover_result result   = previous_block.takeover();
    if (result.front != result.back) {
        size_t thief_counter = thief_block_.load(std::memory_order_relaxed);
        if (thief_counter == predecessor) {
            predecessor += blocks_.size();
            thief_counter += blocks_.size() - 1UL;
            thief_block_.store(thief_counter, std::memory_order_relaxed);
        }
        owner_block_.store(predecessor, std::memory_order_relaxed);
        return true;
    }
    return false;
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::_advance_put_index() noexcept -> bool {
    size_t owner_counter = owner_block_.load(std::memory_order_relaxed);
    size_t next_counter  = owner_counter + 1UL;
    size_t thief_counter = thief_block_.load(std::memory_order_relaxed);
    assert(thief_counter < next_counter);
    if (next_counter - thief_counter >= blocks_.size()) {
        return false;
    }
    size_t next_index   = next_counter & mask_;
    block_t& next_block = blocks_[next_index];
    if (!next_block.is_writable()) [[unlikely]] {
        return false;
    }
    size_t owner_index = owner_counter & mask_;
    block_t& block     = blocks_[owner_index];
    block.grant();
    owner_block_.store(next_counter, std::memory_order_relaxed);
    next_block.reclaim();
    return true;
}

template <typename Ty, _std_simple_allocator Alloc>
CONSTEXPR26 auto lifo_queue<Ty, Alloc>::_advance_steal_index(
    size_t expected_thief_counter) noexcept -> bool {
    size_t thief_counter = expected_thief_counter;
    size_t next_counter  = thief_counter + 1UL;
    size_t next_index    = next_counter & mask_;
    block_t& next_block  = blocks_[next_index];
    if (next_block.is_stealable()) {
        thief_block_.compare_exchange_strong(
            thief_counter, next_counter, std::memory_order_relaxed);
        return true;
    }
    return thief_block_.load(std::memory_order_relaxed) != thief_counter;
}

} // namespace neutron

namespace std {

template <typename Ty, neutron::_std_simple_allocator Alloc>
struct uses_allocator<neutron::lifo_queue<Ty, Alloc>, Alloc> :
    std::true_type {};

} // namespace std
