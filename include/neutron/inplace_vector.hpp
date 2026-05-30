#pragma once
#include <version>

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L

    #include <inplace_vector>

namespace neutron {

template <typename T, std::size_t N>
using inplace_vector = std::inplace_vector<T, N>;

}

#else

    #include <algorithm>
    #include <array>
    #include <cassert>
    #include <cstddef>
    #include <initializer_list>
    #include <iterator>
    #include <memory>
    #include <stdexcept>
    #include <type_traits>
    #include <utility>
    #include "neutron/detail/concepts/nothrow_conditional_movable.hpp"
    #include "neutron/detail/macros.hpp"
    #include "neutron/detail/memory/uninitialized_algorithms.hpp"
    #include "neutron/detail/utility/packed_uint.hpp"

    #if defined(__cpp_lib_containers_ranges) &&                                \
        __cpp_lib_containers_ranges >= 202202L
        #include <ranges>
    #endif

namespace neutron {

namespace internal {

template <typename T, std::size_t N>
concept satisfy_constexpr =
    std::is_trivially_copyable_v<T> && std::is_nothrow_move_constructible_v<T>;

template <typename T, std::size_t N>
struct inplace_vector_trivial_storage {
    using size_type = packed_uint_t<N>;

    ATOM_FORCE_INLINE constexpr T* storage_data() noexcept {
        return _storage.data();
    }

    ATOM_FORCE_INLINE constexpr const T* storage_data() const noexcept {
        return _storage.data();
    }

    size_type _size = 0;
    alignas(alignof(T)) std::array<std::remove_cvref_t<T>, N> _storage{};
};

template <typename T, std::size_t N>
struct inplace_vector_non_trivial_storage { // NOLINT
    using size_type = packed_uint_t<N>;

    ATOM_FORCE_INLINE constexpr T* storage_data() noexcept {
        return reinterpret_cast<T*>(_storage);
    }

    ATOM_FORCE_INLINE constexpr const T* storage_data() const noexcept {
        return reinterpret_cast<T*>(_storage);
    }

    size_type _size = 0;
    alignas(alignof(T)) std::byte _storage[N * sizeof(T)]; // NOLINT
};

template <typename T, std::size_t N>
using storage_for_inplace_vector = typename std::conditional_t<
    satisfy_constexpr<T, N>, inplace_vector_trivial_storage<T, N>,
    inplace_vector_non_trivial_storage<T, N>>;

template <std::size_t N>
struct inplace_container_base {
    ATOM_NODISCARD ATOM_FORCE_INLINE constexpr std::size_t
        capacity() const noexcept {
        return N;
    }

    ATOM_NODISCARD ATOM_FORCE_INLINE constexpr std::size_t
        max_size() const noexcept {
        return N;
    }

    ATOM_FORCE_INLINE constexpr void shrink_to_fit() noexcept {}
};

} // namespace internal

template <typename T, std::size_t N>
class inplace_vector :
    private internal::storage_for_inplace_vector<T, N>,
    public internal::inplace_container_base<N> {
    using _base = internal::storage_for_inplace_vector<T, N>;
    using _base::storage_data;

public:
    using value_type             = T;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr inplace_vector() noexcept = default;

    template <std::input_iterator InputIterator>
    constexpr inplace_vector(InputIterator first, InputIterator last) {
        auto* const pos = uninitialized_copy(first, last, storage_data());
        _base::_size    = std::distance(begin(), pos);
    }

    constexpr inplace_vector(std::initializer_list<T> il) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : inplace_vector(il.begin(), il.end()) {}

    #if ATOM_HAS_CXX23
    template <compatible_range<T> Rng>
    constexpr inplace_vector(std::from_range_t, Rng&& range);
    #endif

    constexpr inplace_vector(const inplace_vector& that)
    requires std::is_trivially_copy_constructible_v<T>
    = default;

    constexpr inplace_vector(const inplace_vector& that) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        uninitialized_copy(that.begin(), that.end(), storage_data());
        _base::_size = that.size();
    }

    constexpr inplace_vector(inplace_vector&& that)
    requires std::is_trivially_move_constructible_v<T>
    = default;

    constexpr inplace_vector(inplace_vector&& that) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        uninitialized_move(that.begin(), that.end(), data());
        _base::size() = that.size();
    }

    constexpr inplace_vector& operator=(const inplace_vector& that)
    requires std::is_trivially_constructible_v<T> &&
                 std::is_trivially_copy_constructible_v<T> &&
                 std::is_trivially_destructible_v<T>
    = default;

    constexpr inplace_vector& operator=(const inplace_vector& that) noexcept(
        std::is_nothrow_copy_assignable_v<T> &&
        std::is_nothrow_copy_constructible_v<T>) {
        //
    }

    constexpr inplace_vector& operator=(inplace_vector&& that)
    requires std::is_trivially_move_assignable_v<T> &&
                 std::is_trivially_move_constructible_v<T> &&
                 std::is_trivially_destructible_v<T>
    = default;

    constexpr inplace_vector& operator=(inplace_vector&& that) noexcept(
        nothrow_conditional_move_constrctible<T> &&
        std::is_nothrow_destructible_v<T>) {}

    constexpr ~inplace_vector()
    requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~inplace_vector() noexcept(std::is_nothrow_destructible_v<T>) {
        clear();
    }

    constexpr void clear() noexcept(std::is_nothrow_destructible_v<T>) {
        std::destroy(begin(), end());
        _base::_size = 0;
    }

    ATOM_NODISCARD constexpr bool empty() const noexcept {
        return _base::_size == 0;
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept {
        return _base::_size;
    }

    constexpr reference operator[](size_type index) noexcept {
        assert(index < storage_size() && "Index out of bounds");
        return storage_data()[index];
    }

    ATOM_NODISCARD constexpr const_reference
        operator[](size_type index) const noexcept {
        assert(index < storage_size() && "Index out of bounds");
        return storage_data()[index];
    }

    constexpr reference at(size_type index) {
        if (index >= _base::_size) {
            throw std::out_of_range("Index out of bounds");
        }
        return storage_data()[index];
    }

    ATOM_NODISCARD constexpr const_reference at(size_type index) const {
        if (index >= _base::size) {
            throw std::out_of_range("Index out of bounds");
        }
        return storage_data()[index];
    }

    constexpr T* data() noexcept { return storage_data(); }

    ATOM_NODISCARD constexpr const T* data() const noexcept {
        return storage_data();
    }

    constexpr iterator begin() noexcept { return storage_data(); }

    ATOM_NODISCARD constexpr const_iterator begin() const noexcept {
        return storage_data();
    }

    constexpr const_iterator cbegin() const noexcept { return storage_data(); }

    constexpr iterator end() noexcept { return storage_data() + _base::_size; }

    ATOM_NODISCARD constexpr const_iterator end() const noexcept {
        return storage_data() + _base::_size;
    }

    constexpr const_iterator cend() const noexcept { return storage_data(); }

    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        auto* const pos = storage_data() + _base::_size;
        std::construct_at(pos, std::forward<Args>(args)...);
        ++_base::_size;
        return *pos;
    }

    constexpr reference push_back(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        return emplace_back(val);
    }

    constexpr reference
        push_back(T&& val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        return emplace_back(std::move(val));
    }

    template <typename... Args>
    constexpr reference emplace(const_iterator pos, Args&&... args) {
        if (_base::_size >= N) [[unlikely]] {
            _throw_bad_alloc();
        }

        auto distance = std::distance(cbegin(), pos);
        iterator iter = begin() + distance;

        std::construct_at(data() + _base::_size, std::forward<Args>(args)...);
        if (_base::_size++) {
            std::rotate(iter, end() - 1, end());
        }
    }

    constexpr iterator insert(const_iterator pos, const T& val) {
        return emplace(pos, val);
    }

    constexpr iterator insert(const_iterator pos, T&& val) {
        return emplace(pos, std::move(val));
    }

    constexpr iterator insert(const_iterator pos, size_type n, const T& val) {
        if (N - _base::_size <= n) [[unlikely]] {
            _throw_bad_alloc();
        }

        auto distance = std::distance(pos, cbegin());
        iterator iter = begin() + distance;
        uninitialized_fill_n(data() + _base::_size, n, val);
        if (std::exchange(_base::_size, _base::_size + n)) {
            std::rotate(iter, end() - n, end());
        }
        return iter;
    }

    template <std::input_iterator InputIterator>
    constexpr iterator
        insert(const_iterator pos, InputIterator first, InputIterator last) {
        auto distance = std::distance(cbegin(), pos);
        iterator iter = begin() + distance;
        //
    }

    #if defined(__cpp_lib_containers_ranges) &&                                \
        __cpp_lib_containers_ranges >= 202202L

    template <compatible_range<T> Rng>
    constexpr iterator insert(const_iterator, Rng&& range) {
        //
    }

    #endif

    constexpr auto pop_back() noexcept(std::is_nothrow_destructible_v<T>);

private:
    ATOM_NORETURN constexpr void _throw_bad_alloc() { throw "bad_alloc"; }
};

template <typename T>
class inplace_vector<T, 0> : public internal::inplace_container_base<0> {
public:
    using value_type             = T;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr inplace_vector() noexcept = default;

    template <std::input_iterator InputIterator>
    constexpr inplace_vector(InputIterator first, InputIterator last) {
        if (first != last) [[unlikely]] {
            throw std::length_error("the size of inplace_vector is zero");
        }
    }

    #if defined(__cpp_lib_containers_ranges) &&                                \
        __cpp_lib_containers_ranges >= 202202L

    template <compatible_range<T> Rng>
    constexpr inplace_vector(std::from_range_t, Rng&& range);

    #endif

    ATOM_NODISCARD constexpr size_type size() const noexcept { return 0; }

    ATOM_NODISCARD constexpr bool empty() const noexcept { return true; }

    constexpr iterator begin() noexcept { return nullptr; }

    constexpr const_iterator begin() const noexcept { return nullptr; }

    constexpr iterator end() noexcept { return nullptr; }

    constexpr const_iterator end() const noexcept { return nullptr; }

    template <typename... Args>
    ATOM_NORETURN constexpr reference emplace_back(Args&&...) {
        _throw_length_error();
    }

    ATOM_NORETURN constexpr reference push_back(const T&) {
        _throw_length_error();
    }

    ATOM_NORETURN constexpr reference push_back(T&&) { _throw_length_error(); }

    ATOM_NORETURN constexpr auto insert(const_iterator, const T&);

    ATOM_NORETURN constexpr auto insert(const_iterator, T&&);

    #if defined(__cpp_lib_containers_ranges) &&                                \
        __cpp_lib_containers_ranges >= 202202L

    template <compatible_range<T> Rng>
    ATOM_NORETURN constexpr auto
        insert(std::from_range_t, const_iterator, Rng&&);

    #endif

private:
    ATOM_NORETURN constexpr void _throw_length_error() {
        throw "try change the size of a zero sized inplace_vector";
    }
};

} // namespace neutron

#endif
