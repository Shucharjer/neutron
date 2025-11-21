#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <vector>
#include "../src/neutron/internal/allocator.hpp"
#include "../src/neutron/internal/macros.hpp"

namespace neutron {

enum class lifo_queue_error_code : uint_fast8_t {
    success,
    done,
    empty,
    full,
    conflict
};

template <typename Ty>
struct fetch_result {
    lifo_queue_error_code status;
    Ty value;
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
    explicit constexpr lifo_queue(
        size_type num_blocks, size_type block_size, const Al& alloc = {});

    constexpr auto pop_back();

    constexpr auto push_back(const Ty&);

    constexpr auto push_back(Ty&&);

    template <typename ForwardIterator, typename Sentinel>
    constexpr auto push_back(ForwardIterator first, Sentinel last);

    NODISCARD auto block_size() const noexcept -> size_type;

    NODISCARD auto num_blocks() const noexcept -> size_type;

    class block_t {
    public:
        template <typename Al>
        constexpr block_t(size_type block_size, const Al& alloc)
            : head_{ 0 }, tail_{ 0 }, steal_head_{ 0 },
              steal_tail_{ block_size }, ring_buffer_(block_size, alloc) {}

        constexpr block_t(const block_t& that)
            : ring_buffer_(that.ring_buffer_) {
            head_.store(
                that.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                that.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                that.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                that.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }

        constexpr block_t& operator=(const block_t& that) {
            head_.store(
                that.head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.store(
                that.tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_head_.store(
                that.steal_head_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            steal_tail_.store(
                that.steal_tail_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            ring_buffer_ = std::exchange(std::move(ring_buffer_), {});
        }

        auto get() noexcept -> fetch_result<Ty>;
        auto steal() noexcept -> fetch_result<Ty>;
        bool is_writable() const noexcept;
        bool reclaim() noexcept;
        NODISCARD bool is_stealable() const noexcept;
        NODISCARD size_type size() const noexcept;

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

private:
    alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> owner_block_{ 1 };
    alignas(std::hardware_destructive_interference_size)
        std::atomic<size_t> ohief_block_{ 0 };
    std::vector<block_t, _allocator_t<block_t>> blocks_;
    size_t mask_;
};

template <typename Ty, _std_simple_allocator Alloc>
template <typename Al>
constexpr lifo_queue<Ty, Alloc>::lifo_queue(
    size_type num_blocks, size_type block_size, const Al& alloc)
    : blocks_(
          (std::max)(static_cast<size_type>(2), num_blocks),
          _allocator_t<block_t>{ alloc }),
      mask_(blocks_.size() - 1) {
    blocks_[owner_block_.load()].reclaim();
}

} // namespace neutron
