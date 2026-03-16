#pragma once
#include <cstddef>
#include <memory>
#include <utility>
#include "neutron/detail/macros.hpp"
#include "neutron/detail/mask.hpp"
#include "neutron/detail/memory/freeable_bytes.hpp"

namespace neutron {

/**
 * @class _pool_proxy
 * @brief A contiguous memory manager.
 * It's designed for container internal implementation and optimization, or you
 * should not use it.
 * @tparam Size Size of each memory block, used to store an object by `take`.
 * @tparam Deletor A functor determines how to delete the contiguous memory
 * block.
 * @note The `void` deletor would do nothing, you should delete the memory
 * block.
 */
template <size_t Size, typename Deletor = void>
class _pool_proxy;

template <size_t Size, typename Deletor>
class _pool_proxy {
public:
    // Check if Deletor supports typed delete: deletor(_free_block<Size>*,
    // size_t)
    constexpr static bool deletor_supports_typed =
        requires(Deletor& del, freeable_bytes<Size>* blocks, size_t capacity) {
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
        : blocks_(static_cast<freeable_bytes<Size>*>(blocks)),
          free_head_(blocks), capacity_(capacity),
          deletor_(std::forward<Dx>(deletor)) {
        _init_uninitialized_freeable_bytes(blocks_, capacity);
    }

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    template <typename Dx = Deletor>
    constexpr _pool_proxy(
        freeable_bytes<Size>* blocks, size_t capacity, Dx&& deletor) noexcept
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
    requires(sizeof(Ty) <= Size)
    constexpr Ty* take() noexcept {
        // To make it constexpr, we use `static_cast` rather than
        // `reinterpret_cast`
        return static_cast<Ty*>(_take_bytes(blocks_, free_head_));
    }

    /**
     * @brief Return a previously acquired block to the pool.
     * @param pointer Pointer to memory obtained via `take`. The pointed-to
     * object must be already destroyed.
     * @note No validation is performed on `ptr`. Caller must ensure validity.
     */
    constexpr void put(void* pointer) noexcept {
        _put_bytes(blocks_, free_head_, capacity_, pointer);
    }

    constexpr auto* data() noexcept { return blocks_; }

    ATOM_NODISCARD constexpr const auto* data() const noexcept {
        return blocks_;
    }

    ATOM_NODISCARD constexpr size_t capacity() const noexcept {
        return capacity_;
    }

private:
    freeable_bytes<Size>* blocks_;
    freeable_bytes<Size>* free_head_;
    size_t capacity_;
    ATOM_NO_UNIQUE_ADDR Deletor deletor_;
};

template <size_t Size>
class _pool_proxy<Size, void> {
public:
    using deletor_type = void;

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    constexpr _pool_proxy(void* blocks, size_t capacity) noexcept
        : blocks_(static_cast<freeable_bytes<Size>*>(blocks)),
          free_head_(blocks_), capacity_(capacity) {
        _init_uninitialized_freeable_bytes<Size>(blocks, capacity);
    }

    /**
     * @warning The alignment of `blocks` should be checked before calling this.
     */
    constexpr _pool_proxy(
        freeable_bytes<Size>* blocks, size_t capacity) noexcept
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
    requires(sizeof(Ty) <= Size)
    constexpr Ty* take() noexcept {
        return static_cast<Ty*>(_take_bytes(blocks_, free_head_));
    }

    /**
     * @brief Return a previously acquired block to the pool.
     * @param pointer Pointer to memory obtained via `take`. The pointed-to
     * object must be already destroyed.
     * @note No validation is performed on `ptr`. Caller must ensure validity.
     */
    constexpr void put(void* pointer) noexcept {
        _put_bytes(blocks_, free_head_, capacity_, pointer);
    }

    constexpr auto* data() noexcept { return blocks_; }

    ATOM_NODISCARD constexpr const auto* data() const noexcept {
        return blocks_;
    }

    ATOM_NODISCARD constexpr size_t capacity() const noexcept {
        return capacity_;
    }

private:
    freeable_bytes<Size>* blocks_;
    freeable_bytes<Size>* free_head_;
    size_t capacity_;
};

/*
 * This implementation could save 8 bytes because of the tprama with larger
 * binary.
 */
template <size_t Size, size_t Capacity, size_t Align = alignof(void*)>
requires _single_bit<Align>
class constcapacity_pool {
public:
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
    constexpr Ty* take() noexcept {
        return std::assume_aligned<Align>(
            static_cast<Ty*>(_take_bytes(blocks_, free_head_)));
    }

    constexpr void put(void* pointer) noexcept {
        _put_bytes(blocks_, free_head_, Capacity, pointer);
    }

private:
    constexpr freeable_bytes<Size>* _as_free(size_t index) noexcept {
        return std::next(blocks_, index);
    }

    alignas(Align) freeable_bytes<Size> blocks_[Capacity]; // NOLINT
    freeable_bytes<Size>* free_head_;
};

template <size_t Size, size_t Align = alignof(void*)>
requires _single_bit<Align>
class runtime_pool {
public:
    explicit constexpr runtime_pool(size_t capacity)
        : proxy_(
              ::operator new(
                  Size* capacity, static_cast<std::align_val_t>(Align)),
              capacity) {}

    runtime_pool(const runtime_pool&)            = delete;
    runtime_pool& operator=(const runtime_pool&) = delete;
    runtime_pool(runtime_pool&&)                 = delete;
    runtime_pool& operator=(runtime_pool&&)      = delete;

    constexpr ~runtime_pool() noexcept {
        ::operator delete(proxy_.data(), static_cast<std::align_val_t>(Align));
    }

    template <typename Ty>
    constexpr Ty* take() noexcept {
        return std::assume_aligned<Align>(proxy_.template take<Ty>());
    }

    constexpr void put(void* pointer) noexcept { proxy_.put(pointer); }

private:
    _pool_proxy<Size> proxy_;
};

} // namespace neutron
