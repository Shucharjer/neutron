// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/using_allocator.hpp"
#include "neutron/detail/utility/exception_guard.hpp"

namespace neutron {

template <typename Ty, std_simple_allocator Alloc = std::allocator<Ty>>
class anchor {
public:
    using value_type       = Ty;
    using allocator_type   = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using size_type        = typename allocator_traits::size_type;
    using difference_type  = typename allocator_traits::difference_type;
    using reference        = value_type&;
    using const_reference  = const value_type&;
    using pointer          = value_type*;
    using const_pointer    = const value_type*;
    using iterator         = _iter_wrapper<pointer>;
    using const_iterator   = _iter_wrapper<const_pointer>;

    static_assert(
        std::is_same_v<typename allocator_traits::pointer, pointer>,
        "anchor requires allocators with raw-pointer storage");

    constexpr anchor() noexcept(
        std::is_nothrow_default_constructible_v<Alloc>) = default;

    template <typename Al = Alloc>
    constexpr explicit anchor(const Al& alloc) noexcept(
        std::is_nothrow_constructible_v<allocator_type, const Al&>)
        : alloc_(alloc) {}

    constexpr anchor(const anchor&)                    = delete;
    constexpr auto operator=(const anchor&) -> anchor& = delete;

    constexpr anchor(anchor&& that) noexcept(
        std::is_nothrow_move_constructible_v<allocator_type>)
        : alloc_(std::move(that.alloc_)), data_(that.data_), size_(that.size_),
          capacity_(that.capacity_) {
        that._reset_storage();
    }

    constexpr auto operator=(anchor&&) -> anchor& = delete;

    constexpr ~anchor() noexcept(std::is_nothrow_destructible_v<value_type>) {
        _release_storage();
    }

    ATOM_NODISCARD constexpr auto get_allocator() const noexcept
        -> allocator_type {
        return alloc_;
    }

    ATOM_NODISCARD constexpr auto data() noexcept -> pointer const& {
        return data_;
    }

    ATOM_NODISCARD constexpr auto data() const noexcept
        -> const_pointer const& {
        return data_;
    }

    ATOM_NODISCARD constexpr auto begin() noexcept -> iterator {
        return iterator{ data_ };
    }

    ATOM_NODISCARD constexpr auto begin() const noexcept -> const_iterator {
        return const_iterator{ data_ };
    }

    ATOM_NODISCARD constexpr auto end() noexcept -> iterator {
        return iterator{ data_ + size_ };
    }

    ATOM_NODISCARD constexpr auto end() const noexcept -> const_iterator {
        return const_iterator{ data_ + size_ };
    }

    ATOM_NODISCARD constexpr auto size() const noexcept -> size_type {
        return size_;
    }

    ATOM_NODISCARD constexpr auto capacity() const noexcept -> size_type {
        return capacity_;
    }

    ATOM_NODISCARD constexpr auto empty() const noexcept -> bool {
        return size_ == 0;
    }

    constexpr auto reserve(size_type count) -> void {
        if (count <= capacity_) {
            return;
        }

        pointer next = allocator_traits::allocate(alloc_, count);
        auto guard   = make_exception_guard([this, next, count]() noexcept {
            allocator_traits::deallocate(alloc_, next, count);
        });

        uninitialized_move_if_noexcept_n_using_allocator(
            alloc_, data_, size_, next);
        guard.mark_complete();

        _destroy_n(data_, size_);
        _deallocate_storage();
        _set_data(next);
        capacity_ = count;
    }

    constexpr auto clear() noexcept -> void {
        _destroy_n(data_, size_);
        size_ = 0;
    }

    constexpr auto push_back(const value_type& value) -> void {
        emplace_back(value);
    }

    constexpr auto push_back(value_type&& value) -> void {
        emplace_back(std::move(value));
    }

    template <typename... Args>
    constexpr auto emplace_back(Args&&... args) -> reference {
        if (size_ == capacity_) {
            reserve(_growth_capacity(size_ + 1));
        }

        allocator_traits::construct(
            alloc_, data_ + size_, std::forward<Args>(args)...);
        ++size_;
        return back();
    }

    constexpr auto pop_back() -> void {
        --size_;
        allocator_traits::destroy(alloc_, data_ + size_);
    }

    ATOM_NODISCARD constexpr auto operator[](size_type index) noexcept
        -> reference {
        return data_[index];
    }

    ATOM_NODISCARD constexpr auto operator[](size_type index) const noexcept
        -> const_reference {
        return data_[index];
    }

    ATOM_NODISCARD constexpr auto back() noexcept -> reference {
        return data_[size_ - 1];
    }

    ATOM_NODISCARD constexpr auto back() const noexcept -> const_reference {
        return data_[size_ - 1];
    }

private:
    constexpr auto _set_data(pointer ptr) noexcept -> void { data_ = ptr; }

    constexpr auto _reset_storage() noexcept -> void {
        _set_data(nullptr);
        size_     = 0;
        capacity_ = 0;
    }

    ATOM_NODISCARD static constexpr auto
        _growth_capacity(size_type min_capacity) noexcept -> size_type {
        if (min_capacity <= 1) [[unlikely]] {
            return 1;
        }

        const size_type doubled = min_capacity << 1;
        return std::min(doubled, min_capacity);
    }

    constexpr auto _destroy_n(pointer ptr, size_type count) noexcept -> void {
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_type index = count; index > 0; --index) {
                allocator_traits::destroy(alloc_, ptr + (index - 1));
            }
        }
    }

    constexpr auto _deallocate_storage() noexcept -> void {
        if (data_ == nullptr) {
            return;
        }

        allocator_traits::deallocate(alloc_, data_, capacity_);
    }

    constexpr auto _release_storage() noexcept -> void {
        _destroy_n(data_, size_);
        _deallocate_storage();
        _reset_storage();
    }

    ATOM_NO_UNIQUE_ADDR allocator_type alloc_{};
    pointer data_{ nullptr };
    size_type size_{ 0 };
    size_type capacity_{ 0 };
};

} // namespace neutron
