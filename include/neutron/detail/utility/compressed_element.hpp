// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */

/**
 * @internal
 * @brief Storage of one element of a compressed class.
 *
 * Empty element types are stored with `ATOM_NO_UNIQUE_ADDR`, so the element
 * itself is empty and takes no space as a base class. Other types are stored
 * as a plain member, keeping the usual layout.
 *
 * All special member functions are defaulted, so the element keeps the
 * triviality of `Ty`.
 *
 * @tparam Ty Element type, may be a reference.
 * @tparam Index Distinguishes elements of the same type.
 */
template <typename Ty, std::size_t Index>
class _compressed_element {
public:
    using value_type      = Ty;
    using reference       = Ty&;
    using const_reference = const Ty&;

    constexpr _compressed_element()
    requires std::is_default_constructible_v<Ty>
    = default;

    template <typename... Args>
    requires std::is_constructible_v<Ty, Args...>
    constexpr explicit _compressed_element(
        std::in_place_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>)
        : value_(std::forward<Args>(args)...) {}

    ATOM_NODISCARD constexpr Ty& value() & noexcept { return value_; }

    ATOM_NODISCARD constexpr const Ty& value() const& noexcept {
        return value_;
    }

    ATOM_NODISCARD constexpr Ty&& value() && noexcept {
        return static_cast<Ty&&>(value_);
    }

    ATOM_NODISCARD constexpr const Ty&& value() const&& noexcept {
        return static_cast<const Ty&&>(value_);
    }

private:
    Ty value_;
};

template <typename Ty, std::size_t Index>
requires std::is_empty_v<Ty>
class _compressed_element<Ty, Index> {
public:
    using value_type      = Ty;
    using reference       = Ty&;
    using const_reference = const Ty&;

    constexpr _compressed_element()
    requires std::is_default_constructible_v<Ty>
    = default;

    template <typename... Args>
    requires std::is_constructible_v<Ty, Args...>
    constexpr explicit _compressed_element(
        std::in_place_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>)
        : value_(std::forward<Args>(args)...) {}

    ATOM_NODISCARD constexpr Ty& value() & noexcept { return value_; }

    ATOM_NODISCARD constexpr const Ty& value() const& noexcept {
        return value_;
    }

    ATOM_NODISCARD constexpr Ty&& value() && noexcept {
        return static_cast<Ty&&>(value_);
    }

    ATOM_NODISCARD constexpr const Ty&& value() const&& noexcept {
        return static_cast<const Ty&&>(value_);
    }

private:
    ATOM_NO_UNIQUE_ADDR Ty value_;
};

/*! @endcond */

} // namespace neutron
