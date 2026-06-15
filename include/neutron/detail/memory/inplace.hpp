// IWYU pragma: private
#pragma once
#include <cstddef>
#include <type_traits>
#include "neutron/detail/macros.hpp"

namespace neutron {

template <typename T, std::size_t Size>
struct inplace_storage;

template <typename T>
struct inplace_storage<T, 0> {
    ATOM_NODISCARD constexpr std::size_t capacity() const noexcept { return 0; }
    ATOM_NODISCARD constexpr std::size_t max_size() const noexcept { return 0; }
    ATOM_NODISCARD constexpr T* data() noexcept { return nullptr; }
    ATOM_NODISCARD constexpr const T* data() const noexcept { return nullptr; }
};

template <typename T, std::size_t Size>
requires std::is_trivially_default_constructible_v<T>
struct inplace_storage<T, Size> {
    T _storage[Size]; // NOLINT(modernize-avoid-c-arrays)

    ATOM_NODISCARD constexpr std::size_t capacity() const noexcept {
        return Size;
    }
    ATOM_NODISCARD constexpr std::size_t max_size() const noexcept {
        return Size;
    }
    ATOM_NODISCARD constexpr T* data() noexcept { return _storage; }
    ATOM_NODISCARD constexpr const T* data() const noexcept { return _storage; }
};

template <typename T, std::size_t Size>
struct inplace_storage {
    struct alignas(T) _block {
        std::byte _[sizeof(T)];       // NOLINT(modernize-avoid-c-arrays)
    };
    alignas(T) _block _storage[Size]; // NOLINT(modernize-avoid-c-arrays)

    ATOM_NODISCARD T* data() noexcept { return reinterpret_cast<T*>(_storage); }
    ATOM_NODISCARD const T* data() const noexcept {
        return reinterpret_cast<const T*>(_storage);
    }
};

} // namespace neutron
