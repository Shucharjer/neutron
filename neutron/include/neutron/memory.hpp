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
#include "neutron/template_list.hpp"
#include "../src/neutron/internal/allocator.hpp"
#include "../src/neutron/internal/utility/exception_guard.hpp"
#include "../src/neutron/internal/utility/immediately.hpp"

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

} // namespace neutron
