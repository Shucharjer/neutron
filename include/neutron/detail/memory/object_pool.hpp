#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include "neutron/detail/mask.hpp"
#include "neutron/detail/memory/freeable_bytes.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @class _pool_proxy
 * @brief A contiguous memory manager.
 * It's designed for container internal implementation and optimization, or you
 * should not use it.
 * @tparam Size Size of each memory block, used to store an object by `take`.
 * @tparam Deletor A functor determines how to delete the contiguous memory
 * block.
 * @tparam Align Alignment of each memory block.
 * @note The `void` deletor would do nothing, you should delete the memory
 * block.
 */
template <
    size_t Size, typename Deletor = void,
    size_t Align = alignof(freeable_bytes<Size>)>
requires _single_bit<Align>
class _pool_proxy;

template <size_t Size, typename Deletor, size_t Align>
requires _single_bit<Align>
class _pool_proxy {
public:
    using block_type = freeable_bytes<Size, Align>;

    // Check if Deletor supports typed delete: deletor(block_type*, size_t)
    constexpr static bool deletor_supports_typed =
        requires(Deletor& del, block_type* blocks, size_t capacity) {
            // You may need to notice the noexcept identifier.
            { del(blocks, capacity) } noexcept;
        };

    // Check if Deletor supports typed delete: deletor(void*, size_t)
    constexpr static bool deletor_supports_void =
        requires(Deletor& del, void* blocks, size_t capacity) {
            // You may need to notice the noexcept identifier.
            { del(blocks, capacity) } noexcept;
        };

    static_assert(
        deletor_supports_typed || deletor_supports_void,
        "the deletor could not delete memory blocks");

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    template <typename Dx = Deletor>
    constexpr _pool_proxy(void* blocks, size_t capacity, Dx&& deletor) noexcept
        : blocks_(static_cast<block_type*>(blocks)), free_head_(blocks_),
          capacity_(capacity), deletor_(std::forward<Dx>(deletor)) {
        _init_uninitialized_freeable_bytes<block_type>(blocks, capacity);
    }

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    template <typename Dx = Deletor>
    constexpr _pool_proxy(
        block_type* blocks, size_t capacity, Dx&& deletor) noexcept
        : blocks_(blocks), free_head_(blocks), capacity_(capacity),
          deletor_(std::forward<Dx>(deletor)) {
        _init_freeable_bytes(blocks_, capacity);
    }

    _pool_proxy(const _pool_proxy&)            = delete;
    _pool_proxy& operator=(const _pool_proxy&) = delete;

    constexpr _pool_proxy(_pool_proxy&& that) noexcept
        : blocks_(std::exchange(that.blocks_, nullptr)),
          free_head_(std::exchange(that.free_head_, nullptr)),
          capacity_(std::exchange(that.capacity_, 0)),
          deletor_(std::move(that.deletor_)) {}

    constexpr _pool_proxy& operator=(_pool_proxy&& that) noexcept {
        if (this != &that) [[likely]] {
            std::swap(blocks_, that.blocks_);
            std::swap(free_head_, that.free_head_);
            std::swap(capacity_, that.capacity_);
            deletor_ = std::move(that.deletor_);
        }
        return *this;
    }

    /// @note Delete a nullptr is a safe action, so do the `Deletor`.
    constexpr ~_pool_proxy() noexcept {
        if constexpr (deletor_supports_typed) {
            deletor_(blocks_, capacity_);
        } else {
            deletor_(static_cast<void*>(blocks_), capacity_);
        }
    }

    /**
     * @brief Acquire a raw memory block suitable for constructing an object of
     * type `Ty`.
     * @return Pointer to uninitialized memory, or nullptr if no block
     * available.
     */
    template <typename Ty>
    requires(sizeof(Ty) <= Size && alignof(Ty) <= Align)
    constexpr Ty* take() noexcept {
        // To make it constexpr, we use `static_cast` rather than
        // `reinterpret_cast`
        return static_cast<Ty*>(_take_bytes(free_head_));
    }

    /**
     * @brief Return a previously acquired block to the pool.
     * @param pointer Pointer to memory obtained via `take`. The pointed-to
     * object must be already destroyed.
     * @note No validation is performed on `ptr`. Caller must ensure validity.
     */
    constexpr void put(void* pointer) noexcept {
        _put_bytes(free_head_, pointer);
    }

    constexpr auto* data() noexcept { return blocks_; }

    ATOM_NODISCARD constexpr const auto* data() const noexcept {
        return blocks_;
    }

    ATOM_NODISCARD constexpr size_t capacity() const noexcept {
        return capacity_;
    }

private:
    block_type* blocks_;
    block_type* free_head_;
    size_t capacity_;
    ATOM_NO_UNIQUE_ADDR Deletor deletor_;
};

template <size_t Size, size_t Align>
requires _single_bit<Align>
class _pool_proxy<Size, void, Align> {
public:
    using deletor_type = void;
    using block_type   = freeable_bytes<Size, Align>;

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    constexpr _pool_proxy(void* blocks, size_t capacity) noexcept
        : blocks_(static_cast<block_type*>(blocks)), free_head_(blocks_),
          capacity_(capacity) {
        _init_uninitialized_freeable_bytes<block_type>(blocks, capacity);
    }

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    constexpr _pool_proxy(block_type* blocks, size_t capacity) noexcept
        : blocks_(blocks), free_head_(blocks_), capacity_(capacity) {
        _init_freeable_bytes(blocks, capacity);
    }

    _pool_proxy(const _pool_proxy&)            = delete;
    _pool_proxy& operator=(const _pool_proxy&) = delete;

    constexpr _pool_proxy(_pool_proxy&& that) noexcept
        : blocks_(std::exchange(that.blocks_, nullptr)),
          free_head_(std::exchange(that.free_head_, nullptr)),
          capacity_(std::exchange(that.capacity_, 0)) {}
    constexpr _pool_proxy& operator=(_pool_proxy&& that) noexcept {
        if (this != &that) [[likely]] {
            std::swap(blocks_, that.blocks_);
            std::swap(free_head_, that.free_head_);
            std::swap(capacity_, that.capacity_);
        }
        return *this;
    }

    constexpr ~_pool_proxy() noexcept = default;

    /**
     * @brief Acquire a raw memory block suitable for constructing an object of
     * type `Ty`.
     * @return Pointer to uninitialized memory, or nullptr if no block
     * available.
     */
    template <typename Ty>
    requires(sizeof(Ty) <= Size && alignof(Ty) <= Align)
    constexpr Ty* take() noexcept {
        return static_cast<Ty*>(_take_bytes(free_head_));
    }

    /**
     * @brief Return a previously acquired block to the pool.
     * @param pointer Pointer to memory obtained via `take`. The pointed-to
     * object must be already destroyed.
     * @note No validation is performed on `ptr`. Caller must ensure validity.
     */
    constexpr void put(void* pointer) noexcept {
        _put_bytes(free_head_, pointer);
    }

    constexpr auto* data() noexcept { return blocks_; }

    ATOM_NODISCARD constexpr const auto* data() const noexcept {
        return blocks_;
    }

    ATOM_NODISCARD constexpr size_t capacity() const noexcept {
        return capacity_;
    }

private:
    block_type* blocks_;
    block_type* free_head_;
    size_t capacity_;
};

/// Block alignment of a pool: at least `Align`, and enough to hold a pointer.
template <size_t Size, size_t Align>
inline constexpr size_t _pool_block_align =
    std::max(Align, alignof(freeable_bytes<Size>));

/*
 * This implementation could save 8 bytes because of the tprama with larger
 * binary.
 */
template <size_t Size, size_t Capacity, size_t Align = alignof(void*)>
requires(_single_bit<Align> && Capacity != 0)
class constcapacity_pool {
    static constexpr size_t block_align = _pool_block_align<Size, Align>;

public:
    using block_type = freeable_bytes<Size, block_align>;

    explicit constexpr constcapacity_pool() noexcept : free_head_(blocks_) {
        for (size_t i = 0; i < Capacity - 1; ++i) {
            _as_free(i)->next = _as_free(i + 1);
        }
        _as_free(Capacity - 1)->next = nullptr;
    }

    constcapacity_pool(const constcapacity_pool&)            = delete;
    constcapacity_pool& operator=(const constcapacity_pool&) = delete;
    constcapacity_pool(constcapacity_pool&&)                 = delete;
    constcapacity_pool& operator=(constcapacity_pool&&)      = delete;

    constexpr ~constcapacity_pool() noexcept = default;

    template <typename Ty>
    requires(sizeof(Ty) <= Size && alignof(Ty) <= block_align)
    constexpr Ty* take() noexcept {
        return std::assume_aligned<block_align>(
            static_cast<Ty*>(_take_bytes(free_head_)));
    }

    constexpr void put(void* pointer) noexcept {
        _put_bytes(free_head_, pointer);
    }

private:
    constexpr block_type* _as_free(size_t index) noexcept {
        return std::next(blocks_, index);
    }

    block_type blocks_[Capacity]; // NOLINT
    block_type* free_head_;
};

template <size_t Size, size_t Align = alignof(void*)>
requires _single_bit<Align>
class runtime_pool {
    static constexpr size_t block_align = _pool_block_align<Size, Align>;
    using proxy_type                    = _pool_proxy<Size, void, block_align>;

public:
    using block_type = typename proxy_type::block_type;

    explicit constexpr runtime_pool(size_t capacity)
        : proxy_(
              ::operator new(
                  sizeof(block_type) * capacity,
                  static_cast<std::align_val_t>(block_align)),
              capacity) {}

    runtime_pool(const runtime_pool&)            = delete;
    runtime_pool& operator=(const runtime_pool&) = delete;
    runtime_pool(runtime_pool&&)                 = delete;
    runtime_pool& operator=(runtime_pool&&)      = delete;

    constexpr ~runtime_pool() noexcept {
        ::operator delete(
            proxy_.data(), static_cast<std::align_val_t>(block_align));
    }

    template <typename Ty>
    requires(sizeof(Ty) <= Size && alignof(Ty) <= block_align)
    constexpr Ty* take() noexcept {
        return std::assume_aligned<block_align>(proxy_.template take<Ty>());
    }

    constexpr void put(void* pointer) noexcept { proxy_.put(pointer); }

private:
    proxy_type proxy_;
};

} // namespace neutron
