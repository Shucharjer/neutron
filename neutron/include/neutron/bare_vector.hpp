#pragma once
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/ranges/concepts.hpp"

namespace neutron {

template <typename Ty>
class bare_vector;

template <typename Ty>
class bare_vector {
public:
    static_assert(sizeof(Ty));

    // clang-format off

    using value_type             = Ty;
    using pointer                = Ty*;
    using const_pointer          = const Ty*;
    using reference              = Ty&;
    using const_reference        = const Ty&;

    using iterator               = _iter_wrapper<pointer>;
    using const_iterator         = _iter_wrapper<const_pointer>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using size_type              = size_t;
    using difference_type        = std::ptrdiff_t;

    template <typename Alloc>
    using _allocator_traits = typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;

    // clang-format on

    constexpr bare_vector() noexcept = default;

    template <typename Alloc>
    constexpr bare_vector(size_type n, Alloc& alloc) {
        auto guard = make_exception_guard([this, &alloc] { _destroy(alloc); });
        if (n > 0) [[likely]] {
            data_ = _allocator_traits<Alloc>::allocate(alloc, n);
            std::uninitialized_construct_using_allocator(data_, alloc);
        }
        guard.mark_complete();
    }

    template <std::input_iterator Iter, typename Sentinel, typename Alloc>
    constexpr bare_vector(Iter first, Sentinel last, Alloc& alloc) {
        _init_with_sentinel(alloc, first, last);
    }

    template <typename T, typename Alloc>
    constexpr bare_vector(std::initializer_list<T> il, Alloc& alloc) {
        _init_with_size(alloc, il.begin(), il.end(), il.size());
    }

#if ATOM_HAS_CXX23
    template <std::ranges::input_range Rng, typename Alloc>
    constexpr bare_vector(std::from_range_t, Rng&& range, Alloc& alloc) {
        if constexpr (
            std::ranges::forward_range<Rng> || std::ranges::sized_range<Rng>) {
            const auto n = static_cast<size_type>(std::ranges::distance(range));
            _init_with_size(
                alloc, std::ranges::begin(range), std::ranges::end(range), n);
        } else {
            _init_with_sentinel(
                alloc, std::ranges::begin(range), std::ranges::end(range));
        }
    }
#endif

    constexpr bare_vector(const bare_vector& that) = delete;

    template <typename Alloc>
    constexpr bare_vector(const bare_vector& that, Alloc& alloc) {
        _init_with_size(that.begin(), that.end(), alloc);
    }

    constexpr bare_vector(bare_vector&& that) noexcept
        : data_(std::exchange(that.data_, nullptr)), size_(that.size_),
          capacity_(that.capacity_) {}

    constexpr bare_vector& operator=(const bare_vector& that) = delete;

    constexpr bare_vector& operator=(bare_vector&& that) noexcept {
        if (this != &that) [[likely]] {
            swap(that);
        }
        return *this;
    }

    constexpr ~bare_vector() noexcept(std::is_nothrow_destructible_v<Ty>) {
        // assert(data_ != nullptr);
    }

    template <typename Alloc>
    constexpr iterator insert(const_iterator pos, const_reference val);

    template <typename Alloc>
    constexpr iterator insert(const_iterator pos, value_type&& val);

    template <typename Alloc, std::input_iterator Iter, typename Sentinel>
    requires(!std::forward_iterator<Iter>) // for better candidate matching
    constexpr iterator
        insert(Alloc& alloc, const_iterator pos, Iter first, Sentinel last) {
        _insert_with_sentinel(alloc, pos, first, last);
    }

    template <typename Alloc, std::forward_iterator Iter, typename Sentinel>
    constexpr iterator
        insert(Alloc& alloc, const_iterator pos, Iter first, Sentinel last) {
        _insert_with_size(alloc, pos, first, last, std::distance(first, last));
    }

    template <typename Alloc, typename Val = value_type>
    constexpr iterator insert(
        Alloc& alloc, const_iterator pos, std::initializer_list<Val> il) {
        _insert_with_size(alloc, pos, il.begin(), il.end(), il.size());
    }

    template <typename Alloc, compatible_range<value_type> Rng>
    constexpr iterator
        insert_range(Alloc& alloc, const_iterator pos, Rng&& range) {
        if constexpr (
            std::ranges::forward_range<Rng> || std::ranges::sized_range<Rng>) {
            const auto n = static_cast<size_type>(std::ranges::distance(range));
            return _insert_with_size(
                alloc, std::ranges::begin(range), std::ranges::end(range), n);
        } else {
            return _insert_with_sentinel(
                alloc, std::ranges::begin(range), std::ranges::end(range));
        }
    }

    template <typename Alloc, compatible_range<value_type> Rng>
    constexpr void append_range(Rng&& range) {
        insert_range(end(), std::forward<Rng>(range));
    }

    template <typename Alloc, typename... Args>
    constexpr auto emplace(Alloc& alloc, const_iterator pos, Args&&... args);

    template <typename Alloc>
    constexpr auto push_back(Alloc& alloc, const_reference val);

    template <typename Alloc>
    constexpr auto push_back(Alloc& alloc, value_type&& val);

    template <typename Alloc, typename... Args>
    constexpr auto emplace_back(Alloc& alloc, Args&&... args) {}

    constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<Ty>);

    constexpr auto erase(const_iterator where) noexcept(
        std::is_nothrow_destructible_v<Ty>);

    template <typename Alloc>
    constexpr void resize(Alloc& alloc, size_type n);

    template <typename Alloc>
    constexpr void resize(Alloc& alloc, size_type n, const_reference val);

    constexpr auto clear() noexcept(std::is_nothrow_destructible_v<Ty>) {
        if constexpr (std::is_trivially_destructible_v<Ty>) {
            std::destroy_n(data_, size_);
        }
        size_ = 0;
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept { return size_; }

    ATOM_NODISCARD constexpr bool empty() const noexcept { return size_ != 0; }

    ATOM_NODISCARD constexpr size_type capacity() const noexcept {
        return capacity_;
    }

    ATOM_NODISCARD constexpr value_type* data() noexcept { return data_; }

    ATOM_NODISCARD constexpr const value_type* data() const noexcept {
        return data_;
    }

    ATOM_NODISCARD constexpr iterator begin() noexcept {
        return _iter_wrapper{ data_ };
    }

    ATOM_NODISCARD constexpr const_iterator begin() const noexcept {
        return _iter_wrapper{ data_ };
    }

    ATOM_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return _iter_wrapper{ data_ };
    }

    ATOM_NODISCARD constexpr iterator end() noexcept {
        return _iter_wrapper{ data_ + size_ };
    }

    ATOM_NODISCARD constexpr const_iterator end() const noexcept {
        return _iter_wrapper{ data_ + size_ };
    }

    ATOM_NODISCARD constexpr const_iterator cend() const noexcept {
        return _iter_wrapper{ data_ + size_ };
    }

    ATOM_NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ });
    }

    ATOM_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ });
    }

    ATOM_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ });
    }

    ATOM_NODISCARD constexpr reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ + size_ });
    }

    ATOM_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ + size_ });
    }

    ATOM_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(_iter_wrapper{ data_ + size_ });
    }

    constexpr void swap(bare_vector& that) noexcept {
        std::swap(data_, that.data_);
        std::swap(size_, that.size_);
        std::swap(capacity_, that.capacity_);
    }

private:
    template <typename Alloc>
    constexpr void
        _destroy(Alloc& alloc) noexcept(std::is_nothrow_destructible_v<Ty>) {
        clear();
        using alloc_traits =
            std::allocator_traits<Alloc>::template rebind_alloc<value_type>;
        alloc_traits::deallocate(alloc, data_, capacity_);
        data_ = nullptr;
    }

    template <typename Alloc, std::input_iterator Iter, typename Sentinel>
    constexpr void
        _init_with_sentinel(Alloc& alloc, Iter first, Sentinel last) {
        auto guard = make_exception_guard([this, &alloc] { _destroy(alloc); });
        for (; first != last; ++first) {
            emplace_back(*first);
        }
        guard.mark_complete();
    }

    template <typename Alloc, std::input_iterator Iter, typename Sentinel>
    constexpr void
        _init_with_size(Alloc& alloc, Iter first, Sentinel last, size_type n) {
        auto guard = make_exception_guard([this, &alloc] { _destroy(alloc); });
        for (; first != last; ++first) {
            emplace_back(*first);
        }
        guard.mark_complete();
    }

    template <typename Alloc, std::input_iterator Iter, typename Sentinel>
    constexpr void
        _insert_with_sentinel(Alloc& alloc, Iter first, Sentinel last);

    template <typename Alloc, std::input_iterator Iter, typename Sentinel>
    constexpr void _insert_with_size(
        Alloc& alloc, Iter first, Sentinel last, size_type n) {}

    Ty* data_{};
    size_t size_{};
    size_t capacity_{};
};

} // namespace neutron
