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
#include <neutron/memory.hpp>
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/concepts/nothrow_conditional_movable.hpp"
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/using_allocator.hpp"
#include "neutron/detail/utility/exception_guard.hpp"

namespace neutron {

template <
    typename Ty, size_t Count = 4,
    std_simple_allocator Alloc = std::allocator<Ty>>
class smvec;

template <typename Ty, size_t Count, std_simple_allocator Alloc>
requires std::same_as<
    typename std::allocator_traits<Alloc>::value_type*,
    typename std::allocator_traits<Alloc>::pointer>
class smvec<Ty, Count, Alloc> {
    static_assert(Count != 0, "Count must be non-zero");

    template <typename T>
    using _allocator_t =
        typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    struct _block {
        alignas(alignof(Ty)) std::byte _[sizeof(Ty)]; // NOLINT
    };

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
    smvec() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) =
        default;

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    explicit smvec(const Al& alloc) noexcept
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(alloc) {}

    // size constructors
    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    explicit smvec(size_type count, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(alloc) {
        if (count == 0) {
            return;
        }
        if (count > Count) {
            data_      = alloc_.allocate(count);
            auto guard = make_exception_guard([this, count] {
                alloc_.deallocate(data_, count);
                _reset_data_ptr();
            });
            uninitialized_value_construct_n_using_allocator(
                alloc_, data_, count);
            guard.mark_complete();
            capacity_ = count;
        } else {
            uninitialized_value_construct_n_using_allocator(
                alloc_, data_, count);
        }
        size_ = count;
    }

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    smvec(size_type count, const Ty& value, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(alloc) {
        if (count == 0) {
            return;
        }
        if (count > Count) {
            data_      = alloc_.allocate(count);
            auto guard = make_exception_guard([this, count] {
                alloc_.deallocate(data_, count);
                _reset_data_ptr();
            });
            uninitialized_fill_n_using_allocator(alloc_, data_, count, value);
            guard.mark_complete();
            capacity_ = count;
        } else {
            uninitialized_fill_n_using_allocator(alloc_, data_, count, value);
        }
        size_ = count;
    }

    template <std::input_iterator InputIter, typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    smvec(InputIter first, InputIter last, const Al& alloc = {})
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if (std::forward_iterator<InputIter>) {
            _init_with_size(first, last);
        } else {
            _init_with_sentinel(first, last);
        }
    }

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    smvec(std::initializer_list<Ty> ilist, const Al& alloc = {})
        : alloc_(allocator_type(alloc)) {
        _init_with_size(ilist.begin(), ilist.end());
    }

    smvec(const smvec& that) : alloc_(that.alloc_) {
        if (that.size_ == 0) {
            return;
        }
        if (that.size_ <= Count) {
            uninitialized_copy_n_using_allocator(
                alloc_, that.data_, size_, data_);
            size_ = that.size_;
        } else {
            data_      = alloc_.allocate(that.size_);
            auto guard = make_exception_guard([this] {
                alloc_.deallocate(data_, capacity_);
                _reset_data_ptr();
                capacity_ = Count;
            });
            uninitialized_copy_n_using_allocator(
                alloc_, that.data_, that.size_, data_);
            guard.mark_complete();
            capacity_ = that.size_;
            size_     = that.size_;
        }
    }

    smvec(smvec&& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty> ||
        std::is_nothrow_copy_constructible_v<Ty>)
        : alloc_(that.alloc_) {
        if (that.size_ == 0) {
            return;
        }
        if (that.size_ <= Count) {
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, that.data_, that.size_, data_);
            size_ = that.size_;
        } else {
            data_     = std::exchange(that.data_, nullptr);
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        }
    }

    template <typename Al = Alloc>
    requires std::convertible_to<Al, allocator_type>
    smvec(smvec&& that, const Al& alloc)
    requires(std::same_as<std::remove_cvref_t<Al>, Alloc>)
        : alloc_(allocator_type(alloc)) {
        if (that.size_ == 0) {
            return;
        }
        bool can_steal = false;
        if (requires { that.alloc_ == alloc_; }) {
            can_steal = (that.alloc_ == alloc_);
        } else if (std::allocator_traits<Al>::is_always_equal::value) {
            can_steal = true;
        }
        if (can_steal && that.size_ > Count && !that._uses_buffer()) {
            data_     = std::exchange(that.data_, nullptr);
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        } else {
            if (that.size_ <= Count) {
                uninitialized_move_if_noexcept_n_using_allocator(
                    alloc_, that.data_, that.size_, data_);
            } else {
                data_      = alloc_.allocate(that.size_);
                auto guard = make_exception_guard([this] {
                    alloc_.deallocate(data_, capacity_);
                    _reset_data_ptr();
                });
                uninitialized_move_if_noexcept_n(that.data_, that.size_, data_);
                guard.mark_complete();
                capacity_ = that.size_;
            }
            size_ = that.size_;
        }
    }

    smvec& operator=(const smvec& that) {
        if (this == &that) {
            return *this;
        }

        smvec temp = that;
        swap(temp);

        return *this;
    }

    smvec& operator=(smvec&& that) noexcept(
        std::is_nothrow_destructible_v<Ty> && nothrow_conditional_movable<Ty>) {
        if (this == &that) {
            return *this;
        }
        if (that._uses_buffer()) {
            std::destroy_n(data_, size_);
            if constexpr (std::allocator_traits<allocator_type>::
                              propagate_on_container_move_assignment::value) {
                if (!_uses_buffer()) {
                    alloc_.deallocate(data_, capacity_);
                    alloc_ = std::move(that).alloc_;
                    _reset_data_ptr();
                }
            }
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, that.data_, that.size_, data_);
        } else {
            std::destroy_n(data_, size_);
            if (!_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
                _reset_data_ptr();
            }
            if constexpr (std::allocator_traits<allocator_type>::
                              propagate_on_container_move_assignment::value) {
                alloc_ = std::move(that).alloc_;
            }
            data_ = that.data_;
            that._reset_data_ptr();
            size_     = std::exchange(that.size_, 0);
            capacity_ = std::exchange(that.capacity_, Count);
        }
        return *this;
    }

    ~smvec() noexcept(std::is_nothrow_destructible_v<Ty>) {
        if (size_ != 0) {
            std::destroy_n(data_, size_);
            if (!_uses_buffer()) {
                alloc_.deallocate(data_, capacity_);
                data_ = reinterpret_cast<pointer>(storage_);
            }
        }
    }

    // basic modifiers
    void push_back(const value_type& val) { emplace_back(val); }
    void push_back(value_type&& val) { emplace_back(std::move(val)); }

    iterator insert(const_iterator pos, const value_type& value) {
        const size_type index =
            (size_ == 0) ? 0 : static_cast<size_type>(pos.base() - data_);
        if (size_ + 1 > capacity_) {
            _reallocate_and_insert_fill(index, 1, value);
        } else {
            _insert_into_existing_fill(index, 1, value);
        }
        return iterator{ data_ + index };
    }

    iterator insert(const_iterator pos, value_type&& value) {
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

    iterator
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
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        return _insert_with_sentinel(pos, first, last);
    }

    iterator insert(const_iterator pos, std::initializer_list<Ty> ilist) {
        return _insert_with_size(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    reference emplace_back(Args&&... args) /* strong grantee */ {
        if (size_ == capacity_) {
            _relocate(capacity_ << 1);
        }
        alloc_traits::construct(
            alloc_, data_ + size_, std::forward<Args>(args)...);
        ++size_;
        return back();
    }

    void pop_back() noexcept(std::is_nothrow_destructible_v<Ty>) {
        assert(size_ != 0);
        (data_ + size_ - 1)->~Ty();
        --size_;
    }

    reference at(size_type index) {
        if (index >= size_) [[unlikely]] {
            throw std::out_of_range(
                "neutron::small_vector: index out of range");
        }
        return data_[index];
    }

    const_reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range(
                "neutron::small_vector: index out of range");
        }
        return data_[index];
    }

    reference operator[](size_type index) noexcept { return data_[index]; }

    const_reference operator[](size_type index) const noexcept {
        return data_[index];
    }

    pointer data() noexcept { return data_; }

    const_pointer data() const noexcept { return data_; }

    void clear() noexcept(std::is_nothrow_destructible_v<Ty>) {
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    ATOM_NODISCARD size_type size() const noexcept { return size_; }
    ATOM_NODISCARD size_type size_in_bytes() const noexcept {
        return size_ * sizeof(Ty);
    }
    ATOM_NODISCARD size_type max_size() const noexcept {
        return (std::min)((std::numeric_limits<size_type>::max)(),
                          static_cast<size_t>(-1) / sizeof(Ty));
    }
    ATOM_NODISCARD bool empty() const noexcept { return size_ == 0; }
    ATOM_NODISCARD reference front() noexcept { return data_[0]; }
    ATOM_NODISCARD const_reference front() const noexcept { return data_[0]; }
    ATOM_NODISCARD reference back() noexcept { return data_[size_ - 1]; }
    ATOM_NODISCARD const_reference back() const noexcept {
        return data_[size_ - 1];
    }

    ATOM_NODISCARD iterator begin() noexcept { return iterator{ data_ }; }
    ATOM_NODISCARD const_iterator begin() const noexcept {
        return const_iterator{ data_ };
    }
    ATOM_NODISCARD iterator end() noexcept { return iterator{ data_ + size_ }; }
    ATOM_NODISCARD const_iterator end() const noexcept {
        return const_iterator{ data_ + size_ };
    }

    void reserve(size_type n) /* strong grantee */ {
        if (capacity_ >= n) {
            return;
        }

        _relocate(n);
    }

    // capacity helpers
    ATOM_NODISCARD size_type capacity() const noexcept { return capacity_; }

    ATOM_NODISCARD allocator_type get_allocator() const noexcept {
        return alloc_;
    }

    void shrink_to_fit() {
        if (!_uses_buffer()) {
            pointer ptr{};
            if (size_ <= Count) {
                ptr = reinterpret_cast<Ty*>(storage_);
            } else {
                ptr = alloc_.allocate(size_);
            }
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, data_, size_, ptr);
            std::destroy_n(data_, size_);
            alloc_.deallocate(data_, capacity_);
            data_     = ptr;
            capacity_ = size_;
        }
    }

    // resize the container; if growing, value-initialize or fill with `value`
    void resize(size_type count) {
        if (count <= size_) {
            std::destroy_n(data_ + count, size_ - count);
            size_ = count;
            return;
        }

        if (count > capacity_) {
            _relocate(count);
        }
        const size_type add = count - size_;
        uninitialized_default_construct_n_using_allocator(
            alloc_, std::to_address(data_) + size_, add);
        size_ = count;
    }

    void resize(size_type count, const Ty& value) {
        if (count <= size_) {
            std::destroy_n(data_ + count, size_ - count);
            size_ = count;
            return;
        }

        if (count > capacity_) {
            _relocate(count);
        }
        const size_type add = count - size_;
        uninitialized_fill_n_using_allocator(alloc_, data_ + size_, add, value);
        size_ = count;
    }

    // erase API (single and range)
    iterator erase(const_iterator pos) {
        if (pos == end()) [[unlikely]] {
            return end();
        }
        const auto index = static_cast<size_type>(pos.base() - data_);
        // shift left by 1
        std::move(data_ + index + 1, data_ + size_, data_ + index);
        std::destroy_at(data_ + size_ - 1);
        --size_;
        return iterator{ data_ + index };
    }

    iterator erase(const_iterator first, const_iterator last) {
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
    void assign(size_type count, const Ty& value) {
        if (count <= Count) {
            _reset_data();
            uninitialized_fill_n_using_allocator(alloc_, data_, count, value);
            size_ = count;
        } else {
            if (capacity_ < count) {
                auto* const ptr = alloc_.allocate(count);
                auto guard =
                    make_exception_guard([this, ptr, count]() noexcept {
                        alloc_.deallocate(ptr, count);
                    });
                uninitialized_fill_n_using_allocator(
                    alloc_, data_, count, value);
                guard.mark_complete();
                std::destroy_n(data_, size_);
                if (!_uses_buffer()) {
                    alloc_.deallocate(data_, capacity_);
                }
                data_     = ptr;
                capacity_ = count;
            } else {
                std::destroy_n(data_, size_);
                uninitialized_fill_n_using_allocator(
                    alloc_, data_, count, value);
            }
            size_ = count;
        }
    }

    template <std::input_iterator InputIt>
    void assign(InputIt first, InputIt last) {
        smvec temp{ first, last, alloc_ };
        swap(temp);
    }

    void assign(std::initializer_list<Ty> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    // swap; prefers pointer swap when not using SBO for both
    void swap(smvec& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty> &&
        std::is_nothrow_swappable_v<Ty>) {
        if (this == &that) {
            return;
        }
        // Fast path: both on heap, just swap pointers/capacity/size and alloc
        if (!_uses_buffer() && !that._uses_buffer()) {
            using std::swap;
            swap(data_, that.data_);
            swap(size_, that.size_);
            swap(capacity_, that.capacity_);
            if (std::allocator_traits<
                    Alloc>::propagate_on_container_swap::value) {
                swap(alloc_, that.alloc_);
            }
            return;
        }
        // Fallback: move-swap via temporary
        smvec tmp = std::move(*this);
        *this     = std::move(that);
        that      = std::move(tmp);
    }

private:
    [[nodiscard]] bool _uses_buffer() const noexcept {
        return data_ == reinterpret_cast<const Ty*>(std::addressof(storage_));
    }

    ATOM_FORCE_INLINE void _reset_data_ptr() noexcept {
        data_ = reinterpret_cast<pointer>(storage_);
    }

    void _reset_data() noexcept(std::is_nothrow_destructible_v<Ty>) {
        if (size_ == 0) {
            return;
        }

        if (!_uses_buffer()) {
            std::destroy_n(data_, size_);
            alloc_.deallocate(data_, capacity_);
            _reset_data_ptr();
            capacity_ = Count;
        } else {
            std::destroy_n(data_, size_);
        }
        size_ = 0;
    }

    void _relocate(size_type capacity) /* strong grantee */ {
        auto* ptr  = alloc_.allocate(capacity);
        auto guard = make_exception_guard([this, ptr, capacity]() noexcept {
            alloc_.deallocate(ptr, capacity);
        });
        uninitialized_move_if_noexcept_n_using_allocator(
            alloc_, data_, size_, ptr);
        guard.mark_complete();
        std::destroy_n(data_, size_);
        if (!_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = capacity;
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
            uninitialized_copy_using_allocator(alloc_, first, last, data_);
        } else {
            uninitialized_copy_using_allocator(alloc_, first, last, data_);
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
            auto* const ptr = alloc_.allocate(count);
            auto guard = make_exception_guard([this, ptr, count]() noexcept {
                alloc_.deallocate(ptr, count);
            });
            uninitialized_copy_using_allocator(alloc_, first, last, ptr);
            guard.mark_complete();
            data_     = ptr;
            capacity_ = count;
        } else {
            uninitialized_copy_using_allocator(alloc_, first, last, data_);
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
            std::move_backward(
                data_ + index, data_ + size_ - count, data_ + size_);
            std::copy(first, last, data_ + index);
        } else {
            const size_type extra = count - tail;
            auto mid              = first;
            std::advance(mid, tail);
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, data_ + index, tail, data_ + index + count);
            std::copy(first, mid, data_ + index);
        }
        size_ += count;
    }

    void _insert_into_existing_fill(
        size_type index, size_type count, const Ty& value) {
        const size_type tail = size_ - index;
        if (count <= tail) {
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, data_ + size_ - count, count, data_ + size_);
            std::move_backward(
                data_ + index, data_ + size_ - count, data_ + size_);
            std::fill_n(data_ + index, count, value);
        } else {
            const size_type extra = count - tail;
            uninitialized_fill_n_using_allocator(
                alloc_, data_ + size_, extra, value);
            uninitialized_move_if_noexcept_n_using_allocator(
                alloc_, data_ + index, tail, data_ + index + count);
            std::fill_n(data_ + index, tail, value);
        }
        size_ += count;
    }

    template <typename Iter>
    void _reallocate_and_insert(
        size_type index, size_type count, Iter first, Iter last) {
        const size_type capacity = (std::max)(capacity_ << 1, size_ + count);
        Ty* const ptr            = alloc_.allocate(capacity);
        auto guard               = make_exception_guard(
            [this, ptr, capacity] { alloc_.deallocate(ptr, capacity); });
        uninitialized_move_if_noexcept_n_using_allocator(
            alloc_, data_, index, ptr);
        auto it = uninitialized_copy_using_allocator(
            alloc_, first, last, ptr + index);
        uninitialized_move_if_noexcept_n_using_allocator(
            alloc_, data_ + index, size_ - index, it);
        guard.mark_complete();
        std::destroy_n(data_, size_);
        if (!_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = capacity;
        size_ += count;
    }

    void _reallocate_and_insert_fill(
        size_type index, size_type count, const Ty& value) {
        const size_type capacity = (std::max)(capacity_ << 1, size_ + count);
        Ty* const ptr            = alloc_.allocate(capacity);
        auto guard               = make_exception_guard(
            [this, ptr, capacity] { alloc_.deallocate(ptr, capacity); });
        uninitialized_move_if_noexcept_n_using_allocator(
            alloc_, data_, index, ptr);
        uninitialized_fill_n_using_allocator(alloc_, ptr + index, count, value);
        uninitialized_move_if_noexcept_n(
            data_ + index, size_ - index, ptr + index + count);
        guard.mark_complete();
        std::destroy_n(data_, size_);
        if (!_uses_buffer()) {
            alloc_.deallocate(data_, capacity_);
        }
        data_     = ptr;
        capacity_ = capacity;
        size_ += count;
    }

    ATOM_NO_UNIQUE_ADDR allocator_type alloc_;
    _block storage_[Count]; // NOLINT
    Ty* data_ = reinterpret_cast<Ty*>(storage_);
    size_t size_{};
    size_t capacity_{ Count };
};

// Note: no bool specialization. We intentionally use the generic template for
// bool elements (no bit-packing). This keeps semantics simple and avoids proxy
// references.

} // namespace neutron
