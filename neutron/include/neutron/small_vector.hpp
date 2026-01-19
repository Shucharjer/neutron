// A minimal yet robust small_vector with small-buffer optimization.
#pragma once
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/nothrow_conditional_movable.hpp"
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/utility/exception_guard.hpp"
#include "neutron/memory.hpp"

namespace neutron {

template <typename Ty, size_t Count, typename Alloc = std::allocator<Ty>>
class small_vector {
    static_assert(Count != 0, "Count must be non-zero");

    template <typename T>
    using _allocator_t =
        typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    union _storage {
        alignas(Ty) std::byte dummy[sizeof(Ty)]{}; // NOLINT
        Ty data;

        constexpr _storage() noexcept                  = default;
        constexpr _storage(const _storage&)            = delete;
        constexpr _storage& operator=(const _storage&) = delete;
        constexpr _storage(_storage&&)                 = delete;
        constexpr _storage& operator=(_storage&&)      = delete;
        // Do not invoke member destructor here; owner manages lifetime
        ~_storage() noexcept {}
    };
    using _truely_alloc = _allocator_t<Ty>;

public:
    using value_type      = Ty;
    using allocator_type  = _allocator_t<Ty>;
    using alloc_traits    = std::allocator_traits<Alloc>;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = Ty*;
    using const_pointer   = const Ty*;
    using iterator        = _iter_wrapper<Ty*>;
    using const_iterator  = _iter_wrapper<const Ty*>;

    // default and allocator constructors
    constexpr small_vector() noexcept(
        std::is_nothrow_default_constructible_v<_truely_alloc>)
        : alloc_() {}

    template <typename Al = Alloc>
    requires std::convertible_to<_truely_alloc, allocator_type>
    constexpr explicit small_vector(const Al& alloc) noexcept
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {}

    // size constructors
    template <typename Al = Alloc>
    requires std::convertible_to<_truely_alloc, allocator_type>
    explicit ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(size_type count, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if (count == 0) {
            return;
        }
        if (count > Count) {
            data_     = alloc_.allocate(count);
            capacity_ = count;
            std::uninitialized_value_construct_n(data_, count);
        } else {
            size_type i = 0;
            auto guard  = make_exception_guard([this, &i]() noexcept {
                std::destroy(&small_data_[0].data, &small_data_[i].data);
            });
            for (; i < count; ++i) {
                ::new (&small_data_[i].data) Ty();
            }
            guard.mark_complete();
            data_     = &small_data_[0].data;
            capacity_ = Count;
        }
        size_ = count;
    }

    template <typename Al = Alloc>
    requires std::convertible_to<_truely_alloc, allocator_type>
    ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(size_type count, const Ty& value, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if (count == 0) {
            return;
        }
        if (count > Count) {
            data_     = alloc_.allocate(count);
            capacity_ = count;
            std::uninitialized_fill_n(data_, count, value);
        } else {
            size_type i = 0;
            auto guard  = make_exception_guard([this, &i]() noexcept {
                std::destroy(&small_data_[0].data, &small_data_[i].data);
            });
            for (; i < count; ++i) {
                ::new (&small_data_[i].data) Ty(value);
            }
            guard.mark_complete();
            data_     = &small_data_[0].data;
            capacity_ = Count;
        }
        size_ = count;
    }

    template <std::input_iterator InputIter, typename Al = Alloc>
    requires std::convertible_to<Al, _truely_alloc>
    constexpr small_vector(
        InputIter first, InputIter last, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if constexpr (std::forward_iterator<InputIter>) {
            _init_with_size(first, last);
        } else {
            _init_with_sentinel(first, last);
        }
    }

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(std::initializer_list<Ty> ilist, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        _init_with_size(ilist.begin(), ilist.end());
    }

    small_vector(const small_vector& that) : alloc_(that.alloc_) {
        if (that.size_ == 0) {
            return;
        }
        if (that.size_ <= Count) {
            for (size_type i = 0; i < that.size_; ++i) {
                ::new (&small_data_[i].data) Ty(that.data_[i]);
            }
            data_ = &small_data_[0].data;
            size_ = that.size_;
        } else {
            data_      = alloc_.allocate(that.size_);
            capacity_  = that.size_;
            auto guard = make_exception_guard([this] {
                alloc_.deallocate(data_, capacity_);
                data_     = nullptr;
                capacity_ = Count;
            });
            std::uninitialized_copy_n(that.data_, that.size_, data_);
            size_ = that.size_;
            guard.mark_complete();
        }
    }

    constexpr small_vector(small_vector&& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty> ||
        std::is_nothrow_copy_constructible_v<Ty>)
        : alloc_(that.alloc_) {
        if (that.size_ == 0) {
            return;
        }
        if (that.size_ <= Count) {
            for (size_type i = 0; i < that.size_; ++i) {
                if constexpr (std::is_nothrow_move_constructible_v<Ty>) {
                    ::new (&small_data_[i].data) Ty(std::move(that.data_[i]));
                } else {
                    ::new (&small_data_[i].data) Ty(that.data_[i]);
                }
            }
            data_ = &small_data_[0].data;
            size_ = that.size_;
        } else {
            data_     = std::exchange(that.data_, nullptr);
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        }
    }

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    constexpr small_vector(small_vector&& that, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if (that.size_ == 0) {
            return;
        }
        bool can_steal = false;
        if constexpr (requires { that.alloc_ == alloc_; }) {
            can_steal = (that.alloc_ == alloc_);
        } else if constexpr (std::allocator_traits<
                                 Al>::is_always_equal::value) {
            can_steal = true;
        }
        if (can_steal && that.size_ > Count && !that._uses_buffer()) {
            data_     = std::exchange(that.data_, nullptr);
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        } else {
            if (that.size_ <= Count) {
                for (size_type i = 0; i < that.size_; ++i) {
                    if constexpr (std::is_nothrow_move_constructible_v<Ty>) {
                        ::new (&small_data_[i].data)
                            Ty(std::move(that.data_[i]));
                    } else {
                        ::new (&small_data_[i].data) Ty(that.data_[i]);
                    }
                }
                data_ = &small_data_[0].data;
                size_ = that.size_;
            } else {
                data_      = alloc_.allocate(that.size_);
                capacity_  = that.size_;
                auto guard = make_exception_guard([this] {
                    alloc_.deallocate(data_, capacity_);
                    data_     = nullptr;
                    capacity_ = Count;
                });
                uninitialized_move_if_noexcept_n(that.data_, that.size_, data_);
                size_ = that.size_;
                guard.mark_complete();
            }
        }
    }

    constexpr small_vector& operator=(const small_vector& that) {
        if (this == &that) {
            return *this;
        }
        if (that.size_ == 0) {
            std::destroy_n(data_, size_);
            if (data_ != nullptr && !_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            data_     = nullptr;
            size_     = 0;
            capacity_ = Count;
            return *this;
        }
        if (that.size_ <= Count) {
            if (data_ != nullptr && !_uses_buffer()) {
                std::destroy_n(data_, size_);
                alloc_.deallocate(data_, capacity_);
                data_     = nullptr;
                capacity_ = Count;
                size_     = 0;
            } else {
                std::destroy_n(data_, size_);
                size_ = 0;
            }
            for (size_type i = 0; i < that.size_; ++i) {
                ::new (&small_data_[i].data) Ty(that.data_[i]);
            }
            data_ = &small_data_[0].data;
            size_ = that.size_;
        } else {
            if (capacity_ < that.size_ || data_ == nullptr || _uses_buffer()) {
                if (data_ != nullptr && !_uses_buffer()) {
                    std::destroy_n(data_, size_);
                    alloc_.deallocate(data_, capacity_);
                } else if (data_ != nullptr) {
                    std::destroy_n(data_, size_);
                }
                data_     = alloc_.allocate(that.size_);
                capacity_ = that.size_;
                size_     = 0;
            } else {
                std::destroy_n(data_, size_);
                size_ = 0;
            }
            auto guard = make_exception_guard([this] {
                alloc_.deallocate(data_, capacity_);
                data_     = nullptr;
                capacity_ = Count;
            });
            std::uninitialized_copy_n(that.data_, that.size_, data_);
            size_ = that.size_;
            guard.mark_complete();
        }
        return *this;
    }

    constexpr small_vector& operator=(small_vector&& that) noexcept {
        if (this == &that) {
            return *this;
        }
        if (data_ != nullptr) {
            std::destroy_n(data_, size_);
            if (!_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
        }
        alloc_ = that.alloc_;
        if (that.size_ <= Count) {
            for (size_type i = 0; i < that.size_; ++i) {
                if constexpr (std::is_nothrow_move_constructible_v<Ty>) {
                    ::new (&small_data_[i].data) Ty(std::move(that.data_[i]));
                } else {
                    ::new (&small_data_[i].data) Ty(that.data_[i]);
                }
            }
            data_ = &small_data_[0].data;
            size_ = that.size_;
        } else {
            data_     = std::exchange(that.data_, nullptr);
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        }
        return *this;
    }

    constexpr ~small_vector() noexcept(std::is_nothrow_destructible_v<Ty>) {
        if (data_ != nullptr) {
            std::destroy_n(data_, size_);
            if (!_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            data_ = nullptr;
        }
    }

    // basic modifiers
    constexpr void push_back(const value_type& val) { emplace_back(val); }
    constexpr void push_back(value_type&& val) { emplace_back(std::move(val)); }

    constexpr iterator insert(const_iterator pos, const value_type& value) {
        const size_type index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        if (size_ + 1 > capacity_) {
            _reallocate_and_insert_fill(index, 1, value);
        } else {
            _insert_into_existing_fill(index, 1, value);
        }
        return iterator{ data_ + index };
    }

    constexpr iterator insert(const_iterator pos, value_type&& value) {
        const size_type index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        if (size_ + 1 > capacity_) {
            // reallocate and move single value
            Ty tmp = std::move(value);
            _reallocate_and_insert(index, 1, &tmp, &tmp + 1);
        } else {
            Ty tmp = std::move(value);
            _insert_into_existing(index, 1, &tmp, &tmp + 1);
        }
        return iterator{ data_ + index };
    }

    constexpr iterator
        insert(const_iterator pos, size_type count, const value_type& value) {
        if (count == 0) {
            return iterator{ const_cast<Ty*>(pos.base()) };
        }
        const size_type index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        if (size_ + count > capacity_) {
            _reallocate_and_insert_fill(index, count, value);
        } else {
            _insert_into_existing_fill(index, count, value);
        }
        return iterator{ data_ + index };
    }

    template <std::input_iterator InputIt>
    constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
        return _insert_with_sentinel(pos, first, last);
    }

    constexpr iterator
        insert(const_iterator pos, std::initializer_list<Ty> ilist) {
        return _insert_with_size(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (data_ == nullptr) [[unlikely]] {
            ::new (&small_data_[0].data) Ty(std::forward<Args>(args)...);
            data_ = &small_data_[0].data;
            size_ = 1;
            return back();
        }
        if (size_ == capacity_) [[unlikely]] {
            const auto new_cap = (capacity_ == 0 ? Count : capacity_ << 1);
            auto* const ptr    = alloc_.allocate(new_cap);
            auto guard         = make_exception_guard(
                [this, ptr, new_cap] { alloc_.deallocate(ptr, new_cap); });
            uninitialized_move_if_noexcept_n(data_, size_, ptr);
            ::new (ptr + size_) Ty(std::forward<Args>(args)...);
            guard.mark_complete();
            std::destroy_n(data_, size_);
            if (!_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            data_     = ptr;
            capacity_ = new_cap;
            ++size_;
        } else [[likely]] {
            ::new (data_ + size_) Ty(std::forward<Args>(args)...);
            ++size_;
        }
        return back();
    }

    constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<Ty>) {
        assert(size_ != 0);
        (data_ + size_ - 1)->~Ty();
        --size_;
    }

    constexpr reference at(size_type index) {
        if (index >= size_) [[unlikely]] {
            throw std::out_of_range(
                "neutron::small_vector: index out of range");
        }
        return data_[index];
    }

    constexpr const_reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range(
                "neutron::small_vector: index out of range");
        }
        return data_[index];
    }

    constexpr reference operator[](size_type index) noexcept {
        return data_[index];
    }

    constexpr const_reference operator[](size_type index) const noexcept {
        return data_[index];
    }

    constexpr pointer data() noexcept { return data_; }

    constexpr const_pointer data() const noexcept { return data_; }

    constexpr void clear() noexcept(std::is_nothrow_destructible_v<Ty>) {
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept { return size_; }
    ATOM_NODISCARD constexpr size_type size_in_bytes() const noexcept {
        return size_ * sizeof(Ty);
    }
    ATOM_NODISCARD constexpr size_type max_size() const noexcept {
        return (std::min)((std::numeric_limits<size_type>::max)(),
                          static_cast<size_t>(-1) / sizeof(Ty));
    }
    ATOM_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }
    ATOM_NODISCARD constexpr reference front() noexcept { return data_[0]; }
    ATOM_NODISCARD constexpr const_reference front() const noexcept {
        return data_[0];
    }
    ATOM_NODISCARD constexpr reference back() noexcept {
        return data_[size_ - 1];
    }
    ATOM_NODISCARD constexpr const_reference back() const noexcept {
        return data_[size_ - 1];
    }

    ATOM_NODISCARD constexpr iterator begin() noexcept {
        return iterator{ data_ };
    }
    ATOM_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator{ data_ };
    }
    ATOM_NODISCARD constexpr iterator end() noexcept {
        return iterator{ data_ + size_ };
    }
    ATOM_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator{ data_ + size_ };
    }

    constexpr void reserve(size_type n)
    requires nothrow_conditional_movable<Ty>
    {
        if (capacity_ >= n) {
            return;
        }

        auto* const ptr = alloc_.allocate(n);
        auto guard =
            make_exception_guard([this, ptr, n] { alloc_.deallocate(ptr, n); });
        uninitialized_move_if_noexcept_n(data_, size_, ptr);
        std::destroy_n(data_, size_);
        if (data_ != nullptr && !_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = n;
        guard.mark_complete();
    }

    constexpr void reserve(size_type n)
    requires(!nothrow_conditional_movable<Ty>)
    {
        if (capacity_ >= n) {
            return;
        }

        auto* const ptr = alloc_.allocate(n);
        Ty* succ        = ptr;
        auto guard = make_exception_guard([this, ptr, n, &succ]() noexcept {
            std::destroy(ptr, succ);
            alloc_.deallocate(ptr, n);
        });
        uninitialized_move_if_noexcept_n(data_, size_, ptr);
        std::destroy_n(data_, size_);
        if (data_ != nullptr && !_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = n;
        guard.mark_complete();
    }

    // capacity helpers
    ATOM_NODISCARD constexpr size_type capacity() const noexcept {
        return capacity_;
    }

    ATOM_NODISCARD allocator_type get_allocator() const noexcept {
        return alloc_;
    }

    // shrink capacity to fit size; if size_ <= Count, move back to SBO
    void shrink_to_fit() {
        if (size_ == 0) {
            // release heap if any
            if (data_ != nullptr && !_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            data_     = nullptr;
            capacity_ = Count;
            return;
        }
        if (_uses_buffer()) {
            return; // already optimal (SBO)
        }
        if (size_ <= Count) {
            // move into small buffer
            size_type idx = 0;
            auto guard    = make_exception_guard([this, idx]() noexcept {
                std::destroy(&small_data_[0].data, &small_data_[idx].data);
            });
            for (; idx < size_; ++idx) {
                ::new (&small_data_[idx].data)
                    Ty(std::move_if_noexcept(data_[idx]));
            }
            guard.mark_complete();
            std::destroy_n(data_, size_);
            alloc_.deallocate(data_, capacity_);
            data_     = &small_data_[0].data;
            capacity_ = Count;
        } else if (capacity_ > size_) {
            // shrink heap allocation to exact fit
            Ty* const ptr = alloc_.allocate(size_);
            auto guard    = make_exception_guard(
                [this, ptr] { alloc_.deallocate(ptr, size_); });
            uninitialized_move_if_noexcept_n(data_, size_, ptr);
            guard.mark_complete();
            std::destroy_n(data_, size_);
            alloc_.deallocate(data_, capacity_);
            data_     = ptr;
            capacity_ = size_;
        }
    }

    // resize the container; if growing, value-initialize or fill with `value`
    constexpr void resize(size_type count) {
        if (count <= size_) {
            std::destroy_n(data_ + count, size_ - count);
            size_ = count;
            return;
        }
        const size_type add = count - size_;

        // relocate to fit
        Ty* ptr = nullptr;
        if (count > capacity_) {
            const size_type capacity = (std::max)(capacity_ << 1, count);
            ptr                      = alloc_.allocate(capacity);
            auto guard               = make_exception_guard(
                [this, ptr, capacity] { alloc_.deallocate(ptr, capacity); });
            uninitialized_move_if_noexcept_n(data_, size_, ptr);
            guard.mark_complete();
            std::destroy_n(data_, size_);
            if (data_ != nullptr && !_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            capacity_ = capacity;
            data_     = ptr;
        }

        // construct new

        ptr           = data_ + size_;
        size_type idx = 0;
        ATOM_TRY {
            for (; idx < add; ++idx) {
                alloc_traits::construct(alloc_, ptr + idx);
            }
        }
        ATOM_CATCH(...) { std::destroy_n(ptr, idx); }
        size_ = count;
    }

    constexpr void resize(size_type count, const Ty& value) {
        if (count <= size_) {
            std::destroy_n(data_ + count, size_ - count);
            size_ = count;
            return;
        }
        const size_type add = count - size_;

        // relocate
        Ty* ptr = nullptr;
        if (count > capacity_) {
            const size_type capacity = (std::max)(capacity_ << 1, count);
            ptr                      = alloc_.allocate(capacity);
            auto guard               = make_exception_guard(
                [this, ptr, capacity] { alloc_.deallocate(ptr, capacity); });
            uninitialized_move_if_noexcept_n(data_, size_, ptr);
            guard.mark_complete();
            std::destroy_n(data_, size_);
            if (data_ != nullptr && !_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
            }
            data_     = ptr;
            capacity_ = capacity;
        }

        ptr           = data_ + size_;
        size_type idx = 0;
        ATOM_TRY {
            for (; idx < add; ++idx) {
                alloc_traits::construct(alloc_, ptr + idx, value);
            }
        }
        ATOM_CATCH(...) { std::destroy_n(ptr, idx); }
        size_ = count;
    }

    // erase API (single and range)
    constexpr iterator erase(const_iterator pos) {
        if (pos == end()) {
            return end();
        }
        const auto index = static_cast<size_type>(pos.base() - data_);
        // shift left by 1
        std::move(data_ + index + 1, data_ + size_, data_ + index);
        std::destroy_at(data_ + (size_ - 1));
        --size_;
        return iterator{ data_ + index };
    }

    constexpr iterator erase(const_iterator first, const_iterator last) {
        if (first == last) {
            return iterator{ const_cast<Ty*>(first.base()) };
        }
        const auto idx_first = static_cast<size_type>(first.base() - data_);
        const auto idx_last  = static_cast<size_type>(last.base() - data_);
        const auto count     = idx_last - idx_first;
        std::move(data_ + idx_last, data_ + size_, data_ + idx_first);
        std::destroy_n(data_ + size_ - count, count);
        size_ -= count;
        return iterator{ data_ + idx_first };
    }

    // assign API
    constexpr void assign(size_type count, const Ty& value) {
        if (count <= Count) {
            // move to SBO
            if (data_ != nullptr && !_uses_buffer()) {
                std::destroy_n(data_, size_);
                alloc_.deallocate(data_, capacity_);
                data_     = nullptr;
                capacity_ = Count;
            } else {
                std::destroy_n(data_, size_);
            }
            size_type idx = 0;
            auto guard    = make_exception_guard([this, &idx]() noexcept {
                std::destroy(&small_data_[0].data, &small_data_[idx].data);
            });
            for (; idx < count; ++idx) {
                ::new (&small_data_[idx].data) Ty(value);
            }
            guard.mark_complete();
            data_ = &small_data_[0].data;
            size_ = count;
        } else {
            if (capacity_ < count || data_ == nullptr || _uses_buffer()) {
                // allocate fresh
                if (data_ != nullptr) {
                    std::destroy_n(data_, size_);
                    if (!_uses_buffer()) {
                        alloc_.deallocate(data_, capacity_);
                    }
                }
                data_     = alloc_.allocate(count);
                capacity_ = count;
            } else {
                std::destroy_n(data_, size_);
            }
            std::uninitialized_fill_n(data_, count, value);
            size_ = count;
        }
    }

    template <std::input_iterator InputIt>
    constexpr void assign(InputIt first, InputIt last) {
        if constexpr (std::forward_iterator<InputIt>) {
            const size_type count =
                static_cast<size_type>(std::distance(first, last));
            if (count <= capacity_) {
                size_type i = 0;
                for (; i < (std::min)(size_, count); ++i, ++first) {
                    data_[i] = *first;
                }
                if (count > size_) {
                    std::uninitialized_copy(first, last, data_ + i);
                } else {
                    std::destroy_n(data_ + count, size_ - count);
                }
                size_ = count;
            } else {
                // need fresh storage (possibly SBO)
                if (count <= Count) {
                    if (data_ != nullptr && !_uses_buffer()) {
                        std::destroy_n(data_, size_);
                        alloc_.deallocate(data_, capacity_);
                        data_     = nullptr;
                        capacity_ = Count;
                    } else {
                        std::destroy_n(data_, size_);
                    }
                    size_type i = 0;
                    auto guard  = make_exception_guard([this, &i]() noexcept {
                        std::destroy(
                            &small_data_[0].data, &small_data_[i].data);
                    });
                    for (; i < count; ++i, ++first) {
                        ::new (&small_data_[i].data) Ty(*first);
                    }
                    guard.mark_complete();
                    data_ = &small_data_[0].data;
                    size_ = count;
                } else {
                    // allocate heap
                    Ty* const ptr = alloc_.allocate(count);
                    auto guard    = make_exception_guard(
                        [this, ptr, count] { alloc_.deallocate(ptr, count); });
                    std::uninitialized_copy(first, last, ptr);
                    std::destroy_n(data_, size_);
                    if (data_ != nullptr && !_uses_buffer()) {
                        alloc_.deallocate(data_, capacity_);
                    }
                    data_     = ptr;
                    capacity_ = count;
                    size_     = count;
                    guard.mark_complete();
                }
            }
        } else {
            // single-pass: clear then append
            clear();
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        }
    }

    constexpr void assign(std::initializer_list<Ty> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    // swap; prefers pointer swap when not using SBO for both
    constexpr void swap(small_vector& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty> &&
        std::is_nothrow_swappable_v<Ty>) {
        if (this == &that) {
            return;
        }
        // Fast path: both on heap, just swap pointers/capacity/size and alloc
        const bool this_heap = (data_ != nullptr) && !_uses_buffer();
        const bool that_heap = (that.data_ != nullptr) && !that._uses_buffer();
        if (this_heap && that_heap) {
            using std::swap;
            swap(data_, that.data_);
            swap(size_, that.size_);
            swap(capacity_, that.capacity_);
            if constexpr (std::allocator_traits<
                              Alloc>::propagate_on_container_swap::value) {
                swap(alloc_, that.alloc_);
            }
            return;
        }
        // Fallback: move-swap via temporary
        small_vector tmp = std::move(*this);
        *this            = std::move(that);
        that             = std::move(tmp);
    }

private:
    [[nodiscard]] bool _uses_buffer() const noexcept {
        return data_ == reinterpret_cast<const Ty*>(&small_data_[0]);
    }

    template <std::input_iterator InputIter>
    void _init_with_sentinel(InputIter first, InputIter last) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template <std::forward_iterator ForwardIter>
    requires std::is_nothrow_constructible_v<Ty, std::iter_value_t<ForwardIter>>
    void _init_with_size(ForwardIter first, ForwardIter last) {
        const auto count = std::distance(first, last);
        if (count == 0) [[unlikely]] {
            return;
        }
        if (static_cast<size_type>(count) > Count) {
            data_     = alloc_.allocate(count);
            capacity_ = count;
            for (size_type i = 0; first != last; ++first, ++i) {
                ::new (data_ + i) Ty(*first);
            }
        } else {
            for (size_type i = 0; i < static_cast<size_type>(count);
                 ++i, ++first) {
                ::new (&small_data_[i].data) Ty(*first);
            }
            data_ = &small_data_[0].data;
        }
        size_ = static_cast<size_type>(count);
    }

    template <std::forward_iterator ForwardIter>
    requires(
        !std::is_nothrow_constructible_v<Ty, std::iter_value_t<ForwardIter>>)
    void _init_with_size(ForwardIter first, ForwardIter last) {
        const auto count = std::distance(first, last);
        if (count == 0) [[unlikely]] {
            return;
        }
        if (static_cast<size_type>(count) > Count) {
            data_      = alloc_.allocate(count);
            capacity_  = count;
            Ty* finish = data_;
            auto guard = make_exception_guard(
                [this, &finish]() noexcept { std::destroy(data_, finish); });
            for (; first != last; ++first, ++finish) { // NOLINT
                ::new (finish) Ty(*first);
            }
            guard.mark_complete();
        } else {
            size_type succ = 0;
            auto guard     = make_exception_guard([this, &succ]() noexcept {
                std::destroy(&small_data_[0].data, &small_data_[succ].data);
            });
            for (; succ < static_cast<size_type>(count); ++first, ++succ) {
                ::new (&small_data_[succ].data) Ty(*first);
            }
            guard.mark_complete();
            data_ = &small_data_[0].data;
        }
        size_ = static_cast<size_type>(count);
    }

    template <std::input_iterator InputIter>
    iterator _insert_with_sentinel(
        const_iterator pos, InputIter first, InputIter last) {
        const size_type start_index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        size_type idx = start_index;
        for (; first != last; ++first, ++idx) {
            insert(iterator{ data_ + idx }, *first);
        }
        return iterator{ data_ + start_index };
    }

    template <std::forward_iterator ForwardIter>
    iterator _insert_with_size(
        const_iterator pos, ForwardIter first, ForwardIter last) {
        const size_type count =
            static_cast<size_type>(std::distance(first, last));
        const size_type index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        if (size_ + count > capacity_) {
            _reallocate_and_insert(index, count, first, last);
        } else {
            _insert_into_existing(index, count, first, last);
        }
        return iterator{ data_ + index };
    }

    template <typename Iter>
    void _insert_into_existing(
        size_type index, size_type count, Iter first, Iter last) {
        const size_type tail = size_ - index;
        if (count <= tail) {
            uninitialized_move_if_noexcept_n(
                data_ + size_ - count, count, data_ + size_);
            std::move_backward(
                data_ + index, data_ + size_ - count, data_ + size_);
            std::copy(first, last, data_ + index);
        } else {
            const size_type extra = count - tail;
            auto mid              = first;
            std::advance(mid, tail);
            uninitialized_move_if_noexcept_n(
                data_ + index, tail, data_ + index + count);
            std::uninitialized_copy_n(mid, extra, data_ + size_);
            std::copy(first, mid, data_ + index);
        }
        size_ += count;
    }

    void _insert_into_existing_fill(
        size_type index, size_type count, const Ty& value) {
        const size_type tail = size_ - index;
        if (count <= tail) {
            uninitialized_move_if_noexcept_n(
                data_ + size_ - count, count, data_ + size_);
            std::move_backward(
                data_ + index, data_ + size_ - count, data_ + size_);
            std::fill_n(data_ + index, count, value);
        } else {
            const size_type extra = count - tail;
            std::uninitialized_fill_n(data_ + size_, extra, value);
            uninitialized_move_if_noexcept_n(
                data_ + index, tail, data_ + index + count);
            std::fill_n(data_ + index, tail, value);
        }
        size_ += count;
    }

    template <typename Iter>
    void _reallocate_and_insert(
        size_type index, size_type count, Iter first, Iter last) {
        const size_type new_cap = (std::max)(capacity_ << 1, size_ + count);
        Ty* const ptr           = alloc_.allocate(new_cap);
        auto guard              = make_exception_guard(
            [this, ptr, new_cap] { alloc_.deallocate(ptr, new_cap); });
        uninitialized_move_if_noexcept_n(data_, index, ptr);
        auto it = std::uninitialized_copy(first, last, ptr + index);
        uninitialized_move_if_noexcept_n(data_ + index, size_ - index, it);
        guard.mark_complete();
        std::destroy_n(data_, size_);
        if (data_ != nullptr && !_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = new_cap;
        size_ += count;
    }

    void _reallocate_and_insert_fill(
        size_type index, size_type count, const Ty& value) {
        const size_type new_cap = (std::max)(capacity_ << 1, size_ + count);
        Ty* const ptr           = alloc_.allocate(new_cap);
        auto guard              = make_exception_guard(
            [this, ptr, new_cap] { alloc_.deallocate(ptr, new_cap); });
        uninitialized_move_if_noexcept_n(data_, index, ptr);
        std::uninitialized_fill_n(ptr + index, count, value);
        uninitialized_move_if_noexcept_n(
            data_ + index, size_ - index, ptr + index + count);
        guard.mark_complete();
        std::destroy_n(data_, size_);
        if (data_ != nullptr && !_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = new_cap;
        size_ += count;
    }

    ATOM_NO_UNIQUE_ADDR _truely_alloc alloc_;
    _storage small_data_[Count]; // NOLINT
    pointer data_ = nullptr;
    size_t size_{};
    size_t capacity_{ Count };
};

// Note: no bool specialization. We intentionally use the generic template for
// bool elements (no bit-packing). This keeps semantics simple and avoids proxy
// references.

} // namespace neutron
