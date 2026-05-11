#pragma once
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <version>

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L

    #include <inplace_vector>

namespace neutron {

template <typename T, std::size_t N>
using inplace_vector = std::inplace_vector<T, N>;

}

#else

    #include <array>
    #include <cstdint>
    #include <type_traits>
    #include "neutron/detail/macros.hpp"
    #include "neutron/detail/utility/packed_uint.hpp"

namespace neutron {

namespace internal {

template <typename T, std::size_t N>
class inplace_vector_base;

template <typename T, std::size_t N>
class inplace_vector_storage {
    friend class inplace_vector_base<T, N>;

public:
private:
};

template <typename T, std::size_t N>
concept satisfy_constexpr =
    std::is_trivially_copyable_v<T> && std::is_nothrow_move_constructible_v<T>;

template <typename T>
struct inplace_vector_zero_sized_storage {
protected:
    using size_type = std::uint8_t;
    static constexpr T* storage_data() noexcept { return nullptr; }
    static constexpr size_type storage_size() noexcept { return 0; }
    static constexpr void
        unsafe_set_size([[maybe_unused]] size_type size) noexcept {
        assert(size == 0 && "Size must be zero for zero-sized storage");
    }
};

template <typename T, std::size_t N>
struct inplace_vector_trivial_storage {
protected:
    using size_type = packed_uint_t<N>;

    constexpr T* storage_data() noexcept { return storage_.data(); }
    constexpr size_type storage_size() const noexcept { return size_; }
    constexpr void unsafe_set_size([[maybe_unused]] size_type size) noexcept {
        assert(size <= N && "Size must not exceed capacity");
        size_ = size;
    }

private:
    size_type size_ = 0;
    alignas(alignof(T)) std::array<std::remove_cvref_t<T>, N> storage_;
};

template <typename T, std::size_t N>
struct inplace_vector_non_trivial_storage {
public:
    constexpr inplace_vector_non_trivial_storage() noexcept = default;
    constexpr inplace_vector_non_trivial_storage(
        const inplace_vector_non_trivial_storage&) noexcept = default;
    constexpr inplace_vector_non_trivial_storage(
        inplace_vector_non_trivial_storage&&) noexcept = default;
    constexpr inplace_vector_non_trivial_storage&
        operator=(const inplace_vector_non_trivial_storage&) noexcept = default;
    constexpr inplace_vector_non_trivial_storage&
        operator=(inplace_vector_non_trivial_storage&&) noexcept = default;

    constexpr ~inplace_vector_non_trivial_storage() noexcept
    requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~inplace_vector_non_trivial_storage() noexcept(
        std::is_nothrow_destructible_v<T>) {
        std::destroy(storage_data(), storage_data() + storage_size());
    }

protected:
    using size_type = packed_uint_t<N>;

    constexpr T* storage_data() noexcept {
        return reinterpret_cast<T*>(storage_);
    }
    constexpr size_type storage_size() const noexcept { return size_; }
    constexpr void unsafe_set_size([[maybe_unused]] size_type size) noexcept {
        assert(size <= N && "Size must not exceed capacity");
        size_ = size;
    }

private:
    size_type size_ = 0;
    alignas(alignof(T)) std::byte storage_[N * sizeof(T)]{}; // NOLINT
};

template <typename T, std::size_t N>
using storage_for_inplace_vector = typename std::conditional_t<
    satisfy_constexpr<T, N>,
    std::conditional_t<
        N == 0, inplace_vector_zero_sized_storage<T>,
        inplace_vector_trivial_storage<T, N>>,
    inplace_vector_non_trivial_storage<T, N>>;

template <typename T, std::size_t N>
class inplace_vector_base : storage_for_inplace_vector<T, N> {
    using _base = storage_for_inplace_vector<T, N>;
    using _base::storage_data;
    using _base::storage_size;
    using _base::unsafe_set_size;

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

    ATOM_NODISCARD constexpr bool empty() const noexcept {
        return storage_size() == 0;
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept {
        return storage_size();
    }

    ATOM_NODISCARD constexpr size_type capacity() const noexcept { return N; }

    ATOM_NODISCARD constexpr size_type max_size() const noexcept { return N; }

    constexpr void shrink_to_fit() noexcept {}

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
        if (index >= storage_size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return storage_data()[index];
    }

    ATOM_NODISCARD constexpr const_reference at(size_type index) const {
        if (index >= storage_size()) {
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

    constexpr iterator end() noexcept {
        return storage_data() + storage_size();
    }

    ATOM_NODISCARD constexpr const_iterator end() const noexcept {
        return storage_data() + storage_size();
    }

private:
};

} // namespace internal

template <typename T, std::size_t N>
class inplace_vector : internal::inplace_vector_base<T, N> {
public:
private:
};

} // namespace neutron

#endif
