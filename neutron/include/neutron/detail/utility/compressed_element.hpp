// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include "../macros.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */

/**
 * @internal
 * @brief Storage element for compressed classes.
 *
 * Implements EBCO (Empty Base Class Optimization) for pair elements
 * @tparam Ty Element type
 * @tparam Index Parameter indicating the num of the element.
 */
template <typename Ty, size_t Index>
class _compressed_element {
public:
    using self_type       = _compressed_element;
    using value_type      = Ty;
    using reference       = Ty&;
    using const_reference = const Ty&;

    /**
     * @brief Default constructor.
     *
     */
    constexpr _compressed_element() noexcept(
        std::is_nothrow_default_constructible_v<Ty>)
    requires std::is_default_constructible_v<Ty>
    = default;

    template <typename... Args>
    requires std::is_constructible_v<Ty, Args...>
    explicit constexpr _compressed_element(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<Ty, Args...>)
        : value_(std::forward<Args>(args)...) {}

    // clang-format off
    constexpr _compressed_element(const _compressed_element&) 
        noexcept(std::is_nothrow_copy_constructible_v<Ty>)      = default;
    constexpr _compressed_element(_compressed_element&& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty>) : value_(std::move(that.value_)) {}
    constexpr _compressed_element& operator=(const _compressed_element&)
        noexcept(std::is_nothrow_copy_assignable_v<Ty>)                    = default;
    constexpr _compressed_element& operator=(_compressed_element&& that) noexcept(
        std::is_nothrow_move_constructible_v<Ty>) {
        if (this != &that) {
            value_ = std::move(that.value_);
        }
        return *this;
    }
    // clang-format on

    constexpr ~_compressed_element() noexcept(
        std::is_nothrow_destructible_v<Ty>) = default;

    /**
     * @brief Get the value of this element.
     *
     */
    ATOM_NODISCARD constexpr reference value() & noexcept { return value_; }

    /**
     * @brief Get the value of this element.
     *
     */
    ATOM_NODISCARD constexpr const_reference value() const& noexcept {
        return value_;
    }

    ATOM_NODISCARD constexpr value_type&& value() && noexcept {
        return std::move(value_);
    }

private:
    Ty value_;
};

/**
 * @brief Compressed element. Specific version for pointer type.
 *
 * @tparam Ty Element type.
 */
template <typename Ty, size_t Index>
class _compressed_element<Ty*, Index> {
public:
    using self_type       = _compressed_element;
    using value_type      = Ty*;
    using reference       = Ty*&;
    using const_reference = const Ty*&;

    constexpr _compressed_element() noexcept : value_(nullptr) {}
    explicit constexpr _compressed_element(std::nullptr_t) noexcept
        : value_(nullptr) {}
    template <typename T>
    explicit constexpr _compressed_element(T* value) noexcept : value_(value) {}
    constexpr _compressed_element(const _compressed_element&) noexcept =
        default;
    constexpr _compressed_element&
        operator=(const _compressed_element&) noexcept = default;
    constexpr ~_compressed_element() noexcept          = default;

    constexpr _compressed_element& operator=(std::nullptr_t) noexcept {
        value_ = nullptr;
        return *this;
    }

    template <typename T>
    constexpr _compressed_element& operator=(T* value) noexcept {
        value_ = value;
        return *this;
    }

    constexpr _compressed_element(_compressed_element&& that) noexcept
        : value_(std::exchange(that.value_, nullptr)) {}

    constexpr _compressed_element&
        operator=(_compressed_element&& that) noexcept {
        value_ = std::exchange(that.value_, nullptr);
        return *this;
    }

    /**
     * @brief Get the value of this element.
     *
     */
    ATOM_NODISCARD constexpr reference value() & noexcept { return value_; }

    /**
     * @brief Get the value of this element.
     *
     */
    ATOM_NODISCARD constexpr auto& value() const& noexcept { return value_; }

    /**
     * @brief Get the value of this element.
     *
     */
    ATOM_NODISCARD constexpr value_type value() && noexcept { return value_; }

private:
    Ty* value_;
};

template <typename Ret, typename... Args, size_t Index>
class _compressed_element<Ret (*)(Args...), Index> {
public:
    using self_type       = _compressed_element;
    using value_type      = Ret (*)(Args...);
    using reference       = value_type&;
    using const_reference = const value_type&;

    constexpr explicit _compressed_element(
        const value_type value = nullptr) noexcept
        : value_(value) {}
    constexpr explicit _compressed_element(std::nullptr_t) noexcept
        : value_(nullptr) {}
    constexpr _compressed_element(const _compressed_element&) noexcept =
        default;
    constexpr _compressed_element(_compressed_element&&) noexcept = default;
    constexpr _compressed_element& operator=(std::nullptr_t) noexcept {
        value_ = nullptr;
        return *this;
    }
    constexpr _compressed_element&
        operator=(const _compressed_element&) noexcept = default;
    constexpr _compressed_element&
        operator=(_compressed_element&& that) noexcept = default;
    constexpr ~_compressed_element() noexcept          = default;

    /**
     * @brief Get the value of this element.
     *
     */
    constexpr reference value() & noexcept { return value_; }

    /**
     * @brief Get the value of this element.
     *
     */
    constexpr const_reference value() const& noexcept { return value_; }

    constexpr value_type value() && noexcept { return value_; }

private:
    value_type value_;
};

/**
 * @brief When the type is void
 *
 * @tparam IsFirst
 */
template <size_t Index>
class _compressed_element<void, Index> {
public:
    using self_type  = _compressed_element;
    using value_type = void;

    constexpr _compressed_element() noexcept                      = default;
    constexpr _compressed_element(const _compressed_element&)     = default;
    constexpr _compressed_element(_compressed_element&&) noexcept = default;
    constexpr _compressed_element&
        operator=(const _compressed_element&) = default;
    constexpr _compressed_element&
        operator=(_compressed_element&&) noexcept = default;
    constexpr ~_compressed_element() noexcept     = default;

    /**
     * @brief You may made something wrong.
     *
     */
    constexpr void value() const noexcept {}
};

/*! @endcond */

} // namespace neutron
