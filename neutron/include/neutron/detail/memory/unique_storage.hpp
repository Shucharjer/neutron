// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#include <concepts>
#include <memory>
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/alloc_traits.hpp"
#include "neutron/detail/utility/exception_guard.hpp"
#include "neutron/detail/utility/immediately.hpp"
#include "neutron/metafn.hpp"

namespace neutron {

struct memstorage {
    template <typename Base>
    struct interface : Base {
        ATOM_NODISCARD void* raw() noexcept {
            return this->template invoke<0>();
        }
        ATOM_NODISCARD const void* const_raw() const noexcept {
            return this->template invoke<1>();
        }
    };

    template <typename Impl>
    using impl = value_list<&Impl::pointer, &Impl::const_pointer>;
};

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
template <typename Ty, std_simple_allocator Alloc = std::allocator<Ty>>
class unique_storage :
    private std::allocator_traits<Alloc>::template rebind_alloc<Ty> {
    template <typename T, std_simple_allocator Al>
    friend class unique_storage;

    using alloc_t =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;
    using traits_t = typename std::allocator_traits<alloc_t>;

    template <typename... Args>
    constexpr void _construct_it(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<Ty, Args...>) {
        auto guard = make_exception_guard([this] {
            traits_deallocate(get_allocator(), ptr_);
            ptr_ = nullptr;
        });
        traits_construct(get_allocator(), ptr_, std::forward<Args>(args)...);
        guard.mark_complete();
    }

    constexpr void _erase_without_clean() noexcept {
        // generally, the two functions shouldn't throw
        traits_destroy(get_allocator(), ptr_);
        traits_deallocate(get_allocator(), ptr_);
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
            ptr_ = traits_allocate(that_alloc);
            _construct_it(std::move(*that.ptr_));
            that._erase();
        }
    }

    template <std_simple_allocator Allocator>
    constexpr unique_storage(unique_storage<Ty, Allocator>&& that)
        : alloc_t(), ptr_(nullptr) {
        if (that.ptr_) {
            ptr_ = traits_allocate(get_allocator());
            _construct_it(std::move(*that.ptr_));
            that._erase();
        }
    }

    template <std_simple_allocator Allocator = alloc_t>
    constexpr unique_storage(
        unique_storage<Ty, Allocator>&& that, const allocator_type& alloc)
        : alloc_t(alloc), ptr_(nullptr) {
        if (that.ptr_) {
            ptr_ = traits_allocate(get_allocator());
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
                        ptr_ = traits_allocate(get_allocator());
                        _construct_it(std::move(*that.ptr_));
                        that._erase_without_clean();
                    }
                }
            }

            if (old_ptr) {
                traits_destroy(old_alloc, old_ptr);
                traits_deallocate(old_alloc, old_ptr);
            }
        }
        return *this;
    }

    template <std_simple_allocator Allocator>
    constexpr unique_storage& operator=(unique_storage<Ty, Allocator>&& that) {
        if (this != &that) [[likely]] {
            if (that.ptr_) {
                if (ptr_) {
                    *ptr_ = std::move(*that.ptr_);
                } else {
                    ptr_ = traits_allocate(get_allocator());
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
        : alloc_t(), ptr_(traits_allocate(get_allocator())) {
        _construct_it();
    }

    constexpr unique_storage(
        std::allocator_arg_t, const allocator_type& alloc, immediately_t)
        : alloc_t(alloc), ptr_(traits_allocate(get_allocator())) {
        _construct_it();
    }

    constexpr unique_storage(immediately_t, const allocator_type& alloc)
        : unique_storage(std::allocator_arg, alloc, immediately_t{}) {}

    // construct with variable arguments

    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    constexpr unique_storage(
        std::allocator_arg_t, const allocator_type& alloc, Args&&... args)
        : alloc_t(alloc), ptr_(traits_allocate(get_allocator())) {
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
            ptr_ = traits_allocate(get_allocator());
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

    ATOM_NODISCARD constexpr void* raw() noexcept {
        return static_cast<void*>(ptr_);
    }

    ATOM_NODISCARD constexpr const void* const_raw() const noexcept {
        return static_cast<const void*>(ptr_);
    }

    ATOM_NODISCARD constexpr decltype(auto) operator*() noexcept {
        return *ptr_;
    }

    ATOM_NODISCARD constexpr decltype(auto) operator*() const noexcept {
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

    ATOM_NODISCARD constexpr auto& get_allocator() noexcept {
        return *static_cast<alloc_t*>(this);
    }

    ATOM_NODISCARD constexpr const auto& get_allocator() const noexcept {
        return *static_cast<const alloc_t*>(this);
    }

private:
    typename traits_t::pointer ptr_;
};

} // namespace neutron
