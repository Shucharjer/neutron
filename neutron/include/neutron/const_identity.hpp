#pragma once
#include <concepts>
#include <utility>

namespace neutron {

/**
 * @class const_identity
 * @brief A wrapper wrapps a type with const.
 * It's mainly used to compate with standard library and
 * optimization.
 * E.g. We usually store `std::pair<const Kty, Ty>` in associate compitable cotainers, but
 * we may modifing the key instead of relocating the key to get a higher performance.
 */
template <typename Ty>
class const_identity {
    Ty value_;

public:
    constexpr const_identity() noexcept(std::is_nothrow_default_constructible_v<Ty>) = default;

    constexpr const_identity(const Ty& val) noexcept(std::is_nothrow_copy_constructible_v<Ty>)
        : value_(val) {}

    constexpr const_identity(Ty&& val) noexcept(std::is_nothrow_move_constructible_v<Ty>)
        : value_(std::move(val)) {}

    constexpr const_identity(const const_identity& that) noexcept(
        std::is_nothrow_copy_constructible_v<Ty>) = default;

    constexpr const_identity& operator=(const const_identity& that) noexcept(
        std::is_nothrow_copy_assignable_v<Ty>) = default;

    constexpr const_identity(const_identity&& that)            = delete;
    constexpr const_identity& operator=(const_identity&& that) = delete;

    constexpr ~const_identity() noexcept(std::is_nothrow_destructible_v<Ty>) = default;

    constexpr operator Ty() const noexcept { return value_; }

    constexpr void swap(const_identity& that) noexcept { std::swap(value_, that.value_); }

    template <typename T>
    requires std::convertible_to<T, Ty>
    constexpr void assign(T&& val) {
        value_ = std::forward<T>(val);
    }
};

template <typename Ty>
class const_identity<const Ty> : public const_identity<Ty> {};

}