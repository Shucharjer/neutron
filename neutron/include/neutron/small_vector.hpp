#pragma once
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/utility/exception_guard.hpp"

namespace neutron {

template <typename Ty, size_t Count, typename Alloc = std::allocator<Ty>>
class small_vector {
    static_assert(Count != 0, "Count must be non-zero");

    template <typename T>
    using _allocator_t =
        typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    struct _storage {
        alignas(Ty) std::byte data[sizeof(Ty)]; // NOLINT
    };
    using _truely_alloc = _allocator_t<_storage>;

public:
    using value_type      = Ty;
    using allocator_type  = _allocator_t<Ty>;
    using alloc_traits    = std::allocator_traits<Alloc>;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;
    using iterator        = _iter_wrapper<Ty*>;
    using const_iterator  = _iter_wrapper<const Ty*>;

    template <typename Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX26 small_vector(const Al& alloc = {}) noexcept
        : alloc_(alloc) {}

    template <std::input_iterator InputIter, typename Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(InputIter first, InputIter last, const Al& alloc = {})
        : alloc_(alloc) {
        if constexpr (std::forward_iterator<InputIter>) {
            _init_with_size(first, last);
        } else {
            _init_with_sentinel(first, last);
        }
    }

    template <typename Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(std::initializer_list<Ty> ilist, const Al& alloc = {})
        : alloc_(alloc) {
        _init_with_size(ilist.begin(), ilist.end());
    }

    ATOM_CONSTEXPR_SINCE_CXX26 small_vector(const small_vector& that)
        : alloc_(that.alloc_) {
        if (that._is_small()) {
            data_ = static_cast<Ty*>(
                static_cast<void*>(alloc_.allocate(that.capacity_)));
            capacity_ = that.capacity_;
        }
        auto guard = make_exception_guard([this, &that] {
            alloc_.deallocate(
                static_cast<_storage*>(static_cast<void*>(data_)),
                that.capacity_);
        });
        std::uninitialized_copy_n(that.data_, that.size_, data_);
        guard.mark_complete();
        size_ = that.size_;
    }

    ATOM_CONSTEXPR_SINCE_CXX26 small_vector(small_vector&& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty>)
        : alloc_(that.alloc_) {
        if (that._is_small()) {
            std::uninitialized_move_n(that.data_, that.size_, data_);
        } else {
            data_          = std::exchange(that.data_, nullptr);
            that.capacity_ = Count;
            that.data_ =
                static_cast<Ty*>(static_cast<void*>(&that.small_data_));
            capacity_ = that.capacity_;
        }
        size_      = that.size_;
        that.size_ = 0;
    }

    template <typename Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX26
        small_vector(small_vector&& that, const Al& alloc = {});

    ATOM_CONSTEXPR_SINCE_CXX26 small_vector&
        operator=(const small_vector& that);

    ATOM_CONSTEXPR_SINCE_CXX26 small_vector&
        operator=(small_vector&& that) noexcept;

    ATOM_CONSTEXPR_SINCE_CXX26 ~small_vector() noexcept(
        std::is_nothrow_destructible_v<Ty>) {
        if (size_ != 0) {
            std::destroy_n(data_, size_);
            size_ = 0;
        }

        if (static_cast<void*>(data_) != &small_data_) {
            alloc_.deallocate(
                static_cast<_storage*>(static_cast<void*>(data_)), capacity_);
            data_ = static_cast<Ty*>(static_cast<void*>(&small_data_));
        }
    }

    ATOM_CONSTEXPR_SINCE_CXX26 iterator
        insert(const_iterator pos, const value_type& value) {
        //
    }

    ATOM_CONSTEXPR_SINCE_CXX26 iterator
        insert(const_iterator pos, value_type&& value) {
        //
    }

    ATOM_CONSTEXPR_SINCE_CXX26 iterator
        insert(const_iterator pos, size_type count, const value_type& value) {
        // uninitialized_value_construct_n
    }

    template <std::input_iterator InputIt>
    ATOM_CONSTEXPR_SINCE_CXX26 iterator
        insert(const_iterator pos, InputIt first, InputIt last) {
        _insert_with_sentinel(pos, first, last);
    }

    ATOM_CONSTEXPR_SINCE_CXX26 iterator
        insert(const_iterator pos, std::initializer_list<Ty> ilist) {
        _insert_with_size(pos, ilist.first(), ilist.end());
    }

    template <typename... Args>
    ATOM_CONSTEXPR_SINCE_CXX26 reference emplace_back(Args&&... args) {
        if (size_ == capacity_) { // need grow
            auto* const ptr = alloc_.allocate(Count << 1);
            auto guard =
                make_exception_guard([this, ptr] { alloc_.deallocate(ptr); });
            std::uninitialized_move_n(data_, Count, ptr);
            std::destroy_n(data_, Count);
            guard.mark_complete();
            data_ = ptr;
        } else {
            ::new (data_ + size_) Ty(std::forward<Args>(args)...);
            ++size_;
        }
    }

    constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<Ty>) {
        assert(size_ != 0);
        (data_ + size_ - 1)->~Ty();
    }

    constexpr reference at(size_type index) {
        if (index >= size_) {
            throw std::out_of_range("");
        }

        return data_[index];
    }

    constexpr const_reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range("index of range");
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

    constexpr void clear() noexcept(std::is_nothrow_destructible_v<Ty>);

    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] constexpr reference front() noexcept { return data_[0]; }

    [[nodiscard]] constexpr const_reference front() const noexcept {
        return data_[0];
    }

    [[nodiscard]] constexpr reference back() noexcept {
        return data_[size_ - 1];
    }

    [[nodiscard]] constexpr const_reference back() const noexcept {
        return data_[size_ - 1];
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator{ data_ };
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator{ data_ };
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator{ data_ + size_ };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator{ data_ + size_ };
    }

    constexpr void reserve(size_type n) {
        if (capacity_ >= n) {
            return;
        }

        auto* const ptr = alloc_.allocate(n);
        auto guard =
            make_exception_guard([this, ptr, n] { alloc_.deallocate(ptr, n); });
        std::uninitialized_move_n(data_, size_, ptr);
        data_ = ptr;
        guard.mark_complete();
    }

private:
    [[nodiscard]] constexpr bool _is_small() const noexcept {
        return static_cast<void*>(data_) == &small_data_[0];
    }

    template <std::input_iterator InputIter>
    constexpr void _init_with_sentinel(InputIter first, InputIter last);

    template <std::forward_iterator ForwardIter>
    constexpr void _init_with_size(ForwardIter first, ForwardIter last) {
        const auto count = std::distance(first, last);
        if (count > Count) {
            data_ =
                static_cast<Ty*>(static_cast<void*>(alloc_.allocate(count)));
        }

        //
    }

    template <std::input_iterator InputIter>
    constexpr iterator _insert_with_sentinel(
        const_iterator pos, InputIter first, InputIter last) {
        //
    }

    template <std::forward_iterator ForwardIter>
    constexpr iterator _insert_with_size(
        const_iterator pos, ForwardIter first, ForwardIter last) {
        const size_type count = last - first;
        //
    }

#ifdef _MSC_VER
    [[msvc::no_unique_address]] _truely_alloc alloc_;
#else
    [[no_unique_address]] _truely_alloc alloc_;
#endif
    _storage small_data_[Count]; // NOLINT
    // NOLINTNEXTLINE, for constexpr
    pointer data_{ static_cast<pointer>(static_cast<void*>(small_data_)) };
    size_t size_{};
    size_t capacity_{ Count };
};

} // namespace neutron
