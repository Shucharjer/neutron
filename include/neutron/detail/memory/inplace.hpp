// IWYU pragma: private
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <
    typename T, std::size_t Size,
    bool Constexpr = std::is_trivially_constructible_v<T>>
struct inplace_storage {
    struct alignas(T) _block {
        std::byte _[sizeof(T)];       // NOLINT(modernize-avoid-c-arrays)
    };
    alignas(T) _block _storage[Size]; // NOLINT(modernize-avoid-c-arrays)

    ATOM_ATTRS(ATOM_NODISCARD) T* data() noexcept { return reinterpret_cast<T*>(_storage); }
    ATOM_ATTRS(ATOM_NODISCARD) const T* data() const noexcept {
        return reinterpret_cast<const T*>(_storage);
    }
};

template <typename T, bool Constexpr>
struct inplace_storage<T, 0, Constexpr> {
    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t capacity() const noexcept { return 0; }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t max_size() const noexcept { return 0; }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr T* data() noexcept { return nullptr; }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr const T* data() const noexcept { return nullptr; }
};

template <typename T, std::size_t Size>
requires(std::is_trivially_default_constructible_v<T> && Size != 0)
struct inplace_storage<T, Size, false> {
    T _storage[Size]; // NOLINT(modernize-avoid-c-arrays)

    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t capacity() const noexcept {
        return Size;
    }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t max_size() const noexcept {
        return Size;
    }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr T* data() noexcept { return _storage; }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr const T* data() const noexcept { return _storage; }
};

template <typename T, std::size_t Size>
requires(std::is_trivially_default_constructible_v<T> && Size != 0)
struct inplace_storage<T, Size, true> {
    T _storage[Size]{}; // NOLINT(modernize-avoid-c-arrays)

    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t capacity() const noexcept {
        return Size;
    }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr std::size_t max_size() const noexcept {
        return Size;
    }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr T* data() noexcept { return _storage; }
    ATOM_ATTRS(ATOM_NODISCARD) constexpr const T* data() const noexcept { return _storage; }
};

} // namespace neutron
