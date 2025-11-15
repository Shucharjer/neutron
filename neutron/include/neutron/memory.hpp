/**
 * @file memory.hpp
 * @author Shucharjer (Shucharjer@outlook.com)
 * @brief
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "neutron/internal/allocator.hpp"
#include "neutron/internal/exception_guard.hpp"
#include "neutron/internal/immediately.hpp"
#include "neutron/internal/mask.hpp"
#include "neutron/template_list.hpp"

namespace neutron {

struct _storage {
    template <typename Base>
    struct interface : Base {
        NODISCARD void* raw() noexcept { return this->template invoke<0>(); }
        NODISCARD const void* const_raw() const noexcept {
            return this->template invoke<1>();
        }
    };

    template <typename Impl>
    using impl = value_list<&Impl::pointer, &Impl::const_pointer>;
};

// NOTE: std::allocator<_>::allocate(size_t) is constexpr since c++20, so the
// follow functions were declaraed with constexpr as possible.

/**
 * @class unique_storage
 * @brief A storage that holds a single object of type `Ty`.
 * This class is designed to be used with allocators, and provides a way to get
 * the pointer from the static virtual table. It's essential for
 * high-performance systems which has type-erased occasion.
 * @tparam Ty The type of the object to be stored.
 * @tparam Alloc The allocator to be used for the storage.
 * @note This class is not thread-safe.
 */
template <typename Ty, _std_simple_allocator Alloc = std::allocator<Ty>>
class unique_storage :
    private std::allocator_traits<Alloc>::template rebind_alloc<Ty> {
    template <typename T, _std_simple_allocator Al>
    friend class unique_storage;

    using alloc_t =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;
    using traits_t = typename std::allocator_traits<alloc_t>;

    template <typename... Args>
    constexpr void _construct_it(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<Ty, Args...>) {
        auto guard = make_exception_guard([this] {
            _deallocate(get_allocator(), ptr_);
            ptr_ = nullptr;
        });
        _construct(get_allocator(), ptr_, std::forward<Args>(args)...);
        guard.mark_complete();
    }

    constexpr void _erase_without_clean() noexcept {
        // generally, the two functions shouldn't throw
        _destroy(get_allocator(), ptr_);
        _deallocate(get_allocator(), ptr_);
    }

    constexpr void _erase() noexcept(noexcept(_erase_without_clean())) {
        _erase_without_clean();
        ptr_ = nullptr;
    }

public:
    using value_type     = Ty;
    using allocator_type = alloc_t;
    using alloc_traits   = std::allocator_traits<alloc_t>;
    using pointer        = typename alloc_traits::pointer;
    using const_pointer  = typename alloc_traits::const_pointer;

    unique_storage(const unique_storage& that)
        : alloc_t(that.get_allocator()),
          ptr_(that.ptr_ ? get_allocator().allocate(1) : nullptr) {
        _construct_it(*that.ptr);
    }

    unique_storage& operator=(const unique_storage& that) = delete;

    constexpr unique_storage(unique_storage&& that) noexcept // since c++17
        : alloc_t(std::move(that.get_allocator())),
          ptr_(std::exchange(that.ptr_, nullptr)) {}

    constexpr unique_storage(unique_storage&& that, const allocator_type& alloc)
        : alloc_t(alloc), ptr_(nullptr) {
        auto& that_alloc = get_allocator();
        if (that_alloc == that.get_allocator()) {
            ptr_ = std::exchange(that.ptr_, nullptr);
        } else {
            ptr_ = _allocate(that_alloc);
            _construct_it(std::move(*that.ptr_));
            that._erase();
        }
    }

    template <_std_simple_allocator Allocator>
    constexpr unique_storage(unique_storage<Ty, Allocator>&& that)
        : alloc_t(), ptr_(nullptr) {
        if (that.ptr_) {
            ptr_ = _allocate(get_allocator());
            _construct_it(std::move(*that.ptr_));
            that._erase();
        }
    }

    template <_std_simple_allocator Allocator = alloc_t>
    constexpr unique_storage(
        unique_storage<Ty, Allocator>&& that, const allocator_type& alloc)
        : alloc_t(alloc), ptr_(nullptr) {
        if (that.ptr_) {
            ptr_ = _allocate(get_allocator());
            _construct_it(std::move(*that.ptr_));
            that._erase();
        }
    }

    constexpr unique_storage& operator=(unique_storage&& that) noexcept(
        std::is_nothrow_destructible_v<Ty> &&
        ((traits_t::propagate_on_container_move_assignment::value &&
          std::is_nothrow_move_assignable_v<alloc_t>) ||
         (!traits_t::propagate_on_container_move_assignment::value &&
          std::is_nothrow_copy_assignable_v<alloc_t>))) {
        if (this != &that) [[likely]] {
            // destroy & deallocate later
            pointer old_ptr   = std::exchange(ptr_, nullptr);
            alloc_t old_alloc = get_allocator();

            auto guard = make_exception_guard([&] { ptr_ = old_ptr; });

            if constexpr (traits_t::propagate_on_container_move_assignment::
                              value) {
                get_allocator() = std::move(that.get_allocator());
                guard.mark_complete();
                std::swap(ptr_, that.ptr_);
            } else {
                if (get_allocator() == that.get_allocator()) {
                    std::swap(ptr_, that.ptr_);
                    guard.mark_complete();
                } else if (that.ptr_) {
                    if (old_ptr) {
                        std::swap(ptr_, old_ptr);
                        *ptr_ = std::move(*that.ptr_);
                    } else {
                        ptr_ = _allocate(get_allocator());
                        _construct_it(std::move(*that.ptr_));
                        that._erase_without_clean();
                    }
                }
            }

            if (old_ptr) {
                _destroy(old_alloc, old_ptr);
                _deallocate(old_alloc, old_ptr);
            }
        }
        return *this;
    }

    template <_std_simple_allocator Allocator>
    constexpr unique_storage& operator=(unique_storage<Ty, Allocator>&& that) {
        if (this != &that) [[likely]] {
            if (that.ptr_) {
                if (ptr_) {
                    *ptr_ = std::move(*that.ptr_);
                } else {
                    ptr_ = _allocate(get_allocator());
                    _construct_it(std::move(*that.ptr_));
                }
                that._erase();
            } else {
                if (ptr_) {
                    _erase();
                }
            }
        }

        return *this;
    }

    constexpr unique_storage() noexcept : alloc_t(), ptr_(nullptr) {}

    // construct with allocator

    constexpr unique_storage(
        std::allocator_arg_t,
        const allocator_type&
            alloc) noexcept(std::is_nothrow_copy_constructible_v<alloc_t>)
        : alloc_t(alloc), ptr_(nullptr) {}

    explicit constexpr unique_storage(const allocator_type& alloc) noexcept(
        std::is_nothrow_copy_constructible_v<alloc_t>)
        : unique_storage(std::allocator_arg, alloc) {}

    // construct immediately

    explicit constexpr unique_storage(immediately_t)
    requires std::default_initializable<alloc_t> &&
                 std::default_initializable<Ty>
        : alloc_t(), ptr_(_allocate(get_allocator())) {
        _construct_it();
    }

    constexpr unique_storage(
        std::allocator_arg_t, const allocator_type& alloc, immediately_t)
        : alloc_t(alloc), ptr_(_allocate(get_allocator())) {
        _construct_it();
    }

    constexpr unique_storage(immediately_t, const allocator_type& alloc)
        : unique_storage(std::allocator_arg, alloc, immediately_t{}) {}

    // construct with variable arguments

    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    constexpr unique_storage(
        std::allocator_arg_t, const allocator_type& alloc, Args&&... args)
        : alloc_t(alloc), ptr_(_allocate(get_allocator())) {
        _construct_it(std::forward<Args>(args)...);
    }

    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    constexpr unique_storage(Args&&... args, const allocator_type& alloc)
        : unique_storage(
              std::allocator_arg, alloc, std::forward<Args>(args)...) {}

    template <typename... Args>
    requires std::is_constructible_v<Ty, Args...>
    explicit constexpr unique_storage(Args&&... args)
        : unique_storage(
              std::allocator_arg, allocator_type{},
              std::forward<Args>(args)...) {}

    // assign from type convertible to 'Ty'.

    template <std::convertible_to<Ty> T>
    constexpr unique_storage& operator=(T&& that) {
        if (ptr_) {
            *ptr_ = std::forward<T>(that);
        } else {
            ptr_ = _allocate(get_allocator());
            _construct_it(std::forward<T>(that));
        }
        return *this;
    }

    // dtor

    constexpr ~unique_storage() noexcept {
        if (ptr_) {
            _erase();
        }
    }

    NODISCARD constexpr void* raw() noexcept {
        return static_cast<void*>(ptr_);
    }

    NODISCARD constexpr const void* const_raw() const noexcept {
        return static_cast<const void*>(ptr_);
    }

    NODISCARD constexpr decltype(auto) operator*() noexcept { return *ptr_; }

    NODISCARD constexpr decltype(auto) operator*() const noexcept {
        return *ptr_;
    }

    constexpr auto operator->() noexcept {
        if constexpr (std::is_pointer_v<typename traits_t::pointer>) {
            return ptr_;
        } else {
            return ptr_.operator->();
        }
    }

    constexpr auto operator->() const noexcept {
        if constexpr (std::is_pointer_v<typename traits_t::pointer>) {
            return ptr_;
        } else {
            return ptr_.operator->();
        }
    }

    constexpr explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    constexpr void swap(unique_storage& that) noexcept {
        if constexpr (traits_t::propagate_on_container_swap::value) {
            std::swap(get_allocator(), that.get_allocator());
        } else {
            assert(get_allocator() == that.get_allocator());
        }
        std::swap(ptr_, that.ptr_);
    }

    NODISCARD constexpr auto& get_allocator() noexcept {
        return *static_cast<alloc_t*>(this);
    }

    NODISCARD constexpr const auto& get_allocator() const noexcept {
        return *static_cast<const alloc_t*>(this);
    }

private:
    typename traits_t::pointer ptr_;
};

template <std::unsigned_integral Uint = uint32_t>
class _counter {
public:
    using value_type  = Uint;
    using atomic_type = std::atomic<value_type>;

    constexpr _counter() noexcept = default;

    CONSTEXPR26 _counter(const _counter& that) : count_(that.count_) {
        if (count_) {
            increase();
        }
    }

    CONSTEXPR26 _counter& operator=(const _counter& that) {
        if (this != &that) [[likely]] {
            if (count_) {
                decrease();
            }
            count_ = that.count_;
            if (count_) {
                increase();
            }
        }
        return *this;
    }

    constexpr _counter(_counter&& that) noexcept
        : count_(std::exchange(that.count_, nullptr)) {}

    CONSTEXPR26 _counter& operator=(_counter&& that) noexcept {
        if (this != &that) [[likely]] {
            if (count_) {
                decrease();
            }
            count_ = std::exchange(that.count_, nullptr);
        }
        return *this;
    }

    CONSTEXPR26 ~_counter() {
        if (count_) {
            decrease();
        }
    }

    CONSTEXPR26 void make() { count_ = new atomic_type{}; }

    CONSTEXPR26 void increase() noexcept {
        count_->fetch_add(1, std::memory_order_relaxed);
    }

    CONSTEXPR26 void decrease() noexcept {
        auto current = count_->fetch_sub(1, std::memory_order_relaxed);
        if (current == 1) {
            delete count_;
        }
    }

    CONSTEXPR26 auto get() noexcept {
        return count_->load(std::memory_order_relaxed);
    }

private:
    atomic_type* count_{};
};

/**
 * @class shared_storage
 * @brief A COW container.
 */
template <typename Ty, _std_simple_allocator Alloc>
class shared_storage :
    private std::allocator_traits<Alloc>::template rebind_alloc<Ty> {
public:
    // clang-format off

    using alloc_t        = typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;
    using traits_t       = std::allocator_traits<alloc_t>;
    using _counter_type  = _counter<uint32_t>;

    using allocator_type = alloc_t;
    using alloc_traits   = traits_t;

    using value_type     = typename alloc_traits::value_type;
    using pointer        = typename alloc_traits::pointer;
    using const_pointer  = typename alloc_traits::const_pointer;

private:
    template <typename... Args>
    constexpr void _construct_it(Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>) {
        auto guard = make_exception_guard([this]{
            _deallocate(get_allocator(), ptr_);
            ptr_ = nullptr;
        });
        _construct(get_allocator(), ptr_, std::forward<Args>(args)...);
        guard.mark_complete();
    }

public:
    // clang-format on

    constexpr shared_storage() noexcept : alloc_t(), ptr_() {}

    constexpr shared_storage(
        std::allocator_arg_t, const allocator_type& alloc) noexcept
        : alloc_t(alloc), ptr_() {}

    constexpr shared_storage(const allocator_type& alloc) noexcept
        : shared_storage(std::allocator_arg, alloc) {}

    shared_storage(
        std::allocator_arg_t, const allocator_type& alloc, immediately_t)
        : alloc_t(alloc), ptr_(_allocate(get_allocator())) {
        _construct_it();
        counter_.make();
    }

    shared_storage(immediately_t)
        : shared_storage(std::allocator_arg, alloc_t{}, immediately) {}

    shared_storage(immediately_t, const allocator_type& alloc)
        : shared_storage(std::allocator_arg, alloc_t{}, immediately) {}

    template <typename... Args>
    shared_storage(
        std::allocator_arg_t, const allocator_type& alloc, Args&&... args)
        : alloc_t(alloc), ptr_(_allocate(get_allocator())) {
        _construct_it(std::forward<Args>(args)...);
        counter_.make();
    }

    template <typename... Args>
    shared_storage(Args&&... args, const allocator_type& alloc)
        : shared_storage(
              std::allocator_arg, alloc, std::forward<Args>(args)...) {}

    template <typename... Args>
    shared_storage(Args&&... args)
        : shared_storage(
              std::allocator_arg, alloc_t{}, std::forward<Args>(args)...) {}

    constexpr shared_storage(const shared_storage& that);

    constexpr shared_storage& operator=(const shared_storage& that);

    constexpr shared_storage(shared_storage&& that) noexcept;

    constexpr shared_storage(
        shared_storage&& that, const allocator_type& alloc);

    constexpr shared_storage& operator=(shared_storage&& that) noexcept;

    constexpr ~shared_storage() noexcept;

    template <std::convertible_to<Ty> T>
    shared_storage& operator=(T&& that);

    constexpr decltype(auto) operator*() noexcept { return *ptr_; }

    constexpr decltype(auto) operator*() const noexcept { return *ptr_; }

    constexpr auto operator->() noexcept { return ptr_; }

    constexpr auto operator->() const noexcept { return ptr_; }

    constexpr auto count() noexcept { return counter_.get(); }

    constexpr alloc_t& get_allocator() noexcept {
        return *static_cast<alloc_t*>(this);
    }

    constexpr const alloc_t& get_allocator() const noexcept {
        return *static_cast<const alloc_t*>(this);
    }

private:
    pointer ptr_;
    _counter_type counter_;
};

template <typename Ty>
struct alignas(alignof(Ty)) _type_block {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    std::byte bytes[sizeof(Ty)];
};

template <size_t Size>
using packed_size_t = std::conditional_t<
    (Size < std::numeric_limits<uint8_t>::max()), uint8_t,
    std::conditional_t<
        (Size < std::numeric_limits<uint16_t>::max()), uint16_t,
        std::conditional_t<
            (Size < std::numeric_limits<uint32_t>::max()), uint32_t,
            uint64_t>>>;

template <size_t Size>
using packed_uint_t = std::conditional_t<
    (Size <= std::numeric_limits<uint8_t>::max()), uint8_t,
    std::conditional_t<
        (Size <= std::numeric_limits<uint16_t>::max()), uint16_t,
        std::conditional_t<
            (Size <= std::numeric_limits<uint32_t>::max()), uint32_t,
            uint64_t>>>;

template <size_t Size>
using fast_packed_size_t = std::conditional_t<
    (Size < std::numeric_limits<uint8_t>::max()), uint_fast8_t,
    std::conditional_t<
        (Size < std::numeric_limits<uint16_t>::max()), uint_fast16_t,
        std::conditional_t<
            (Size < std::numeric_limits<uint32_t>::max()), uint_fast32_t,
            uint_fast64_t>>>;

template <size_t Size>
using fast_packed_uint_t = std::conditional_t<
    (Size <= std::numeric_limits<uint8_t>::max()), uint_fast8_t,
    std::conditional_t<
        (Size <= std::numeric_limits<uint16_t>::max()), uint_fast16_t,
        std::conditional_t<
            (Size <= std::numeric_limits<uint32_t>::max()), uint_fast32_t,
            uint_fast64_t>>>;

/**
 * @brief A union stores bytes or next free memory block.
 * The size of a union equals to the largest element.
 * @tparam Size
 */
template <size_t Size>
union freeable_bytes {
    freeable_bytes* next;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    std::byte bytes[Size];
};

template <size_t Size>
constexpr void _init_freeable_bytes(
    freeable_bytes<Size>* blocks, size_t capacity) noexcept {
    assert(capacity != 0);
    for (size_t i = 0; i < capacity - 1; i++) {
        auto* const block = std::launder(std::next(blocks, i));
        block->next       = std::next(blocks, i + 1);
    }
    std::next(blocks, capacity - 1)->next = nullptr;
}

template <size_t Size>
constexpr void
    _init_uninitialized_freeable_bytes(void* blocks, size_t capacity) noexcept {
    static_assert(
        std::is_trivially_default_constructible_v<freeable_bytes<Size>>);
    _init_freeable_bytes<Size>(
        static_cast<freeable_bytes<Size>*>(blocks), capacity);
}

template <size_t Size>
constexpr void* _take_bytes(
    freeable_bytes<Size>* blocks, freeable_bytes<Size>*& head) noexcept {
    if (head == nullptr) [[unlikely]] {
        return nullptr;
    }
    auto* const block = head;
    head              = head->next;
    return block;
}

template <size_t Size>
constexpr void _put_bytes(
    freeable_bytes<Size>* blocks, freeable_bytes<Size>*& head, size_t capacity,
    void* pointer) noexcept {
    if (pointer == nullptr) [[unlikely]] {
        return;
    }
    auto* const block = static_cast<freeable_bytes<Size>*>(pointer);
    block->next       = head;
    head              = block;
}

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

    NODISCARD constexpr const auto* data() const noexcept { return blocks_; }

    NODISCARD constexpr size_t capacity() const noexcept { return capacity_; }

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

    NODISCARD constexpr const auto* data() const noexcept { return blocks_; }

    NODISCARD constexpr size_t capacity() const noexcept { return capacity_; }

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
