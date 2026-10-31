#pragma once
#include <atomic>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "neutron/detail/utility/macros.hpp"

namespace neutron::internal {

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct _lockfree_node {
    std::atomic<_lockfree_node*> next;
    alignas(T) std::byte bytes[sizeof(T)]; // NOLINT

    template <typename... Args>
    T* construct(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        return std::construct_at(
            reinterpret_cast<T*>(static_cast<std::byte*>(bytes)),
            std::forward<Args>(args)...);
    }

    void destroy() noexcept {
        std::destroy_at(reinterpret_cast<T*>(static_cast<std::byte*>(bytes)));
    }

    /// @brief Accesses the stored value, which must have been constructed.
    ATOM_NODISCARD T* value() noexcept {
        return std::launder(
            reinterpret_cast<T*>(static_cast<std::byte*>(bytes)));
    }

    /// @brief Accesses the stored value, which must have been constructed.
    ATOM_NODISCARD const T* value() const noexcept {
        return std::launder(
            reinterpret_cast<const T*>(static_cast<const std::byte*>(bytes)));
    }
};

} // namespace neutron::internal
