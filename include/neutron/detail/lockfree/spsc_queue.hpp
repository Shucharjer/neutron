#pragma once
#include <algorithm>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/utility/completion_guard.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief A growable Single-Producer Single-Consumer (SPSC) lock-free queue.
 *
 * Exactly one thread may call `push_back`, and exactly one (other) thread may
 * call `front`, `empty` and `pop_front`. The consumer inspects the front value
 * through `front` and consumes it in place with `pop_front`, so values never
 * need to be moved or copied. `front` and `pop_front` require a non-empty
 * queue, which the consumer usually ensures through `empty`.
 *
 * Values live in ring buffers (blocks) that form a circular list. Every slot
 * carries a sequence number next to the value, which tells whether the slot is
 * free or holds the value of a given position, so the two ends never share an
 * index: the consumer polls the slot it is about to read, and the producer
 * probes slots ahead in batches. In the steady state, an element costs one
 * release store and one acquire load on the cache line holding the value.
 *
 * When the producer fills its block, it moves to the next block if the
 * consumer has left it, and otherwise links a new block, which is twice as
 * large as the largest block so far up to a limit. The consumer only moves to
 * the next block after the producer has published an element there. Blocks are
 * never released before destruction.
 *
 * @tparam T The type of the stored values.
 * @tparam Alloc The allocator type, rebound to the internal blocks.
 */
template <typename T, typename Alloc = std::allocator<T>>
class spsc_queue {
    static_assert(
        std::is_nothrow_destructible_v<T>,
        "spsc_queue requires nothrow destructible values");

public:
    using value_type     = T;
    using allocator_type = rebind_alloc_t<Alloc, T>;
    using alloc_traits   = std::allocator_traits<allocator_type>;
    using size_type      = std::size_t;

private:
    /**
     * @brief A slot for the value at position `i`, where `i & mask` indexes
     * the slot.
     *
     * `seq == i` means the slot is free for position `i`, and `seq == i + 1`
     * means it holds the value at position `i`. Consuming position `i` frees
     * the slot for position `i + mask + 1`.
     */
    struct _slot_t {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        explicit _slot_t(size_type index) noexcept : seq(index) {}

        ATOM_NODISCARD auto raw() noexcept -> T* {
            return reinterpret_cast<T*>(static_cast<std::byte*>(bytes));
        }

        ATOM_NODISCARD auto get() noexcept -> T* { return std::launder(raw()); }

        std::atomic<size_type> seq;
        alignas(T) std::byte bytes[sizeof(T)]; // NOLINT
    };

    struct _block_t {
        /// @brief Only changes while this block is the producer's block.
        std::atomic<_block_t*> next{ nullptr };
        typename std::allocator_traits<rebind_alloc_t<Alloc, _slot_t>>::pointer
            storage{};
        _slot_t* slots{ nullptr };
        size_type mask{ 0 };
        size_type front{ 0 }; ///< Where the consumer left. Consumer-owned.
        size_type tail{ 0 };  ///< Where the producer left. Producer-owned.
    };

    using _slot_alloc_t       = rebind_alloc_t<Alloc, _slot_t>;
    using _slot_alloc_traits  = std::allocator_traits<_slot_alloc_t>;
    using _block_alloc_t      = rebind_alloc_t<Alloc, _block_t>;
    using _block_alloc_traits = std::allocator_traits<_block_alloc_t>;

    /// @brief One end inside its block, which owns positions `[index, limit)`.
    struct _cursor_t {
        _block_t* block{ nullptr };
        _slot_t* slots{ nullptr };
        size_type mask{ 0 };
        size_type index{ 0 };
        size_type limit{ 0 };
    };

    // At least two slots, so that "free for `i + 1`" differs from "holds `i`".
    static constexpr size_type _min_capacity = 16;
    static constexpr size_type _max_growth   = std::bit_floor(
        (std::max)(_min_capacity, (size_type{ 1 } << 20) / sizeof(_slot_t)));
    static_assert(_min_capacity >= 2);

public:
    /**
     * @param cap The number of elements the first block holds at least. The
     * queue grows beyond it when the producer outruns the consumer.
     * @param alloc The allocator used for the blocks.
     */
    template <typename Al = Alloc>
    requires std::constructible_from<allocator_type, const Al&>
    explicit spsc_queue(size_type cap, const Al& alloc = {}) : alloc_(alloc) {
        const size_type capacity =
            std::bit_ceil((std::max)(cap, _min_capacity));
        _block_t* const block = _allocate_block(capacity);
        block->next.store(block, std::memory_order_relaxed);

        prod_    = _cursor_t{ block, block->slots, block->mask, 0, capacity };
        cons_    = _cursor_t{ block, block->slots, block->mask, 0, 0 };
        largest_ = capacity;
        tail_block_.store(block, std::memory_order_relaxed);
        front_block_.store(block, std::memory_order_relaxed);
    }

    spsc_queue(const spsc_queue&)            = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;
    spsc_queue(spsc_queue&&)                 = delete;
    spsc_queue& operator=(spsc_queue&&)      = delete;

    ~spsc_queue() noexcept {
        // Blocks from the consumer's to the producer's hold the elements.
        _block_t* block = cons_.block;
        size_type index = cons_.index;
        while (true) {
            const bool last     = block == prod_.block;
            const size_type end = last ? prod_.index : block->tail;
            for (; index != end; ++index) {
                alloc_traits::destroy(
                    alloc_, block->slots[index & block->mask].get());
            }
            if (last) {
                break;
            }
            block = block->next.load(std::memory_order_relaxed);
            index = block->front;
        }

        _block_t* const first = block;
        do {
            _block_t* const next = block->next.load(std::memory_order_relaxed);
            _deallocate_block(block);
            block = next;
        } while (block != first);
    }

    /**
     * @brief Checks whether the queue is empty.
     *
     * Only the consumer thread may call this function. A producer in the middle
     * of `push_back` can make it report `true` for an element that is on its
     * way.
     */
    ATOM_NODISCARD auto empty() const noexcept -> bool {
        if (cons_.index != cons_.limit || _ready()) [[likely]] {
            return false;
        }
        // The producer only leaves a block after publishing an element in the
        // next one.
        return tail_block_.load(std::memory_order_acquire) == cons_.block;
    }

    /**
     * @brief Constructs a value at the back of the queue.
     *
     * Only the producer thread may call this function.
     */
    template <typename... Args>
    requires std::constructible_from<T, Args...>
    void push_back(Args&&... args) {
        if (prod_.index != prod_.limit) [[likely]] {
            _emplace(std::forward<Args>(args)...);
            return;
        }
        _push_back_slow(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns the value at the front of the queue.
     *
     * Only the consumer thread may call this function, and the queue must not
     * be empty. The reference stays valid until `pop_front`; pushes never
     * invalidate it.
     */
    ATOM_NODISCARD auto front() noexcept -> T& { return *_front_slot().get(); }

    /// @copydoc front()
    ATOM_NODISCARD auto front() const noexcept -> const T& {
        return *_front_slot().get();
    }

    /**
     * @brief Destroys the front value.
     *
     * Only the consumer thread may call this function, and the queue must not
     * be empty.
     */
    void pop_front() noexcept {
        _slot_t& slot = _front_slot();
        alloc_traits::destroy(alloc_, slot.get());
        ++cons_.index;
        slot.seq.store(cons_.index + cons_.mask, std::memory_order_release);
    }

    ATOM_NODISCARD auto get_allocator() const noexcept -> allocator_type {
        return alloc_;
    }

private:
    auto _allocate_block(size_type capacity) -> _block_t* {
        _slot_alloc_t slot_alloc(alloc_);
        _block_alloc_t block_alloc(alloc_);
        const auto storage = _slot_alloc_traits::allocate(slot_alloc, capacity);
        auto guard =
            make_exception_guard([&slot_alloc, &storage, capacity]() noexcept {
                _slot_alloc_traits::deallocate(slot_alloc, storage, capacity);
            });
        _block_t* const block =
            std::to_address(_block_alloc_traits::allocate(block_alloc, 1));
        guard.dismiss();

        _slot_t* const slots = std::to_address(storage);
        for (size_type i = 0; i != capacity; ++i) {
            std::construct_at(slots + i, i);
        }
        std::construct_at(block);
        block->storage = storage;
        block->slots   = slots;
        block->mask    = capacity - 1;
        return block;
    }

    void _deallocate_block(_block_t* block) noexcept {
        using block_pointer = typename _block_alloc_traits::pointer;
        _slot_alloc_t slot_alloc(alloc_);
        _block_alloc_t block_alloc(alloc_);
        const size_type capacity = block->mask + 1;
        std::destroy_n(block->slots, capacity);
        _slot_alloc_traits::deallocate(slot_alloc, block->storage, capacity);
        std::destroy_at(block);
        _block_alloc_traits::deallocate(
            block_alloc, std::pointer_traits<block_pointer>::pointer_to(*block),
            1);
    }

    /// @note The producer must own a free slot.
    template <typename... Args>
    ATOM_ALWAYS_INLINE void _emplace(Args&&... args) {
        _slot_t& slot = prod_.slots[prod_.index & prod_.mask];
        alloc_traits::construct(
            alloc_, slot.raw(), std::forward<Args>(args)...);
        slot.seq.store(++prod_.index, std::memory_order_release);
    }

    /**
     * @brief Extends the free positions owned by the producer.
     *
     * The consumer frees slots in order, so a free slot implies that every
     * slot before it is free as well. Probes far ahead first to claim a large
     * batch, and halves the distance on failure.
     */
    auto _probe() noexcept -> bool {
        const size_type index = prod_.index;
        for (size_type n = (prod_.mask + 1) / 2; n != 0; n /= 2) {
            const size_type last = index + n - 1;
            if (prod_.slots[last & prod_.mask].seq.load(
                    std::memory_order_acquire) == last) {
                prod_.limit = index + n;
                return true;
            }
        }
        return false;
    }

    template <typename... Args>
    ATOM_NOINLINE void _push_back_slow(Args&&... args) {
        if (_probe()) {
            _emplace(std::forward<Args>(args)...);
            return;
        }

        // The block is full. The next block is free unless the consumer is
        // still in it.
        _block_t* const block = prod_.block;
        _block_t* next        = block->next.load(std::memory_order_relaxed);
        if (next == front_block_.load(std::memory_order_acquire)) {
            const size_type capacity =
                largest_ < _max_growth ? largest_ * 2 : largest_;
            _block_t* const fresh = _allocate_block(capacity);
            largest_              = capacity;
            // An empty block is harmless to the consumer, so link it before the
            // construction, which may throw.
            fresh->next.store(next, std::memory_order_relaxed);
            block->next.store(fresh, std::memory_order_release);
            next = fresh;
        }

        const _cursor_t prev = prod_;
        // The consumer left `next` empty where the producer left it.
        prod_ = _cursor_t{ next, next->slots, next->mask, next->tail,
                           next->tail + next->mask + 1 };
        auto guard =
            make_exception_guard<std::is_nothrow_constructible_v<T, Args...>>(
                [this, &prev]() noexcept {
                    prod_ = prev;
                });
        _emplace(std::forward<Args>(args)...);
        guard.dismiss();

        block->tail = prev.index;
        // Publish the block after its first element, so that a consumer that
        // sees the producer leave a block always finds an element ahead.
        tail_block_.store(next, std::memory_order_release);
    }

    /// @brief Checks whether the consumer's current slot holds a value.
    ATOM_ALWAYS_INLINE auto _ready() const noexcept -> bool {
        const size_type index = cons_.index;
        if (cons_.slots[index & cons_.mask].seq.load(
                std::memory_order_acquire) == index + 1) {
            cons_.limit = index + 1;
            return true;
        }
        return false;
    }

    ATOM_ALWAYS_INLINE auto _front_slot() const noexcept -> _slot_t& {
        if (cons_.index == cons_.limit) [[unlikely]] {
            _refill();
        }
        return cons_.slots[cons_.index & cons_.mask];
    }

    /// @note The queue must not be empty.
    ATOM_NOINLINE void _refill() const noexcept {
        if (_ready()) {
            return;
        }

        // The block is drained, so the producer has moved on. Synchronize with
        // it and recheck the slot, which may have been filled meanwhile.
        [[maybe_unused]] _block_t* const tail =
            tail_block_.load(std::memory_order_acquire);
        NEUTRON_ASSERT(tail != cons_.block);
        if (_ready()) {
            return;
        }

        _block_t* const block = cons_.block;
        _block_t* const next  = block->next.load(std::memory_order_acquire);
        block->front          = cons_.index;
        // Hand the drained block back to the producer.
        front_block_.store(next, std::memory_order_release);
        cons_ = _cursor_t{ next, next->slots, next->mask, next->front,
                           next->front };
        [[maybe_unused]] const bool ready = _ready();
        NEUTRON_ASSERT(ready);
    }

    /// @brief Producer-owned state.
    alignas(hdi_size) _cursor_t prod_;
    size_type largest_{ 0 }; ///< The capacity of the largest block.

    /// @brief Consumer-owned state, advanced lazily by the const observers.
    alignas(hdi_size) mutable _cursor_t cons_;

    /// @brief The block the producer writes to.
    alignas(hdi_size) std::atomic<_block_t*> tail_block_{ nullptr };
    /// @brief The block the consumer reads from.
    mutable std::atomic<_block_t*> front_block_{ nullptr };
    ATOM_NO_UNIQUE_ADDR allocator_type alloc_;
};

} // namespace neutron
