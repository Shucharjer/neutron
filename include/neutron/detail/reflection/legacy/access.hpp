// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include "neutron/tstring.hpp"
#include "./concepts.hpp"
#include "./member_count.hpp"
#include "./member_names.hpp"
#include "./get.hpp"

namespace neutron {

/**
 * @brief Get the existance of a member.
 *
 */
template <tstring Name, reflectible Ty>
[[nodiscard]] consteval auto existance_of() noexcept {
    constexpr auto count = member_count_of<Ty>();
    constexpr auto names = member_names_of<Ty>();
    for (auto i = 0; i < count; ++i) {
        if (names[i] == Name.val) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get the existance of a member.
 *
 */
template <reflectible Ty>
[[nodiscard]] constexpr auto existance_of(std::string_view name) noexcept
    -> size_t {
    constexpr auto count = member_count_of<Ty>();
    auto names           = member_names_of<Ty>();
    for (auto i = 0; i < count; ++i) {
        if (names[i] == name) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get the index of a member.
 *
 */
template <tstring Name, reflectible Ty>
[[nodiscard]] consteval auto index_of() noexcept -> size_t {
    constexpr auto count = member_count_of<Ty>();
    constexpr auto names = member_names_of<Ty>();
    for (auto i = 0; i < count; ++i) {
        if (names[i] == Name.data()) {
            return i;
        }
    }
    return static_cast<std::size_t>(-1);
}

/**
 * @brief Get the index of a member.
 *
 */
template <reflectible Ty>
[[nodiscard]] constexpr auto index_of(std::string_view name) noexcept
    -> size_t {
    constexpr auto count = member_count_of<Ty>();
    auto names           = member_names_of<Ty>();
    for (auto i = 0; i < count; ++i) {
        if (names[i] == name) {
            return i;
        }
    }
    return static_cast<std::size_t>(-1);
}

template <size_t Index, reflectible Ty>
[[nodiscard]] consteval bool valid_index() noexcept {
    return Index < member_count_of<Ty>();
}

template <reflectible Ty>
[[nodiscard]] constexpr bool valid_index(const size_t index) noexcept {
    return index < member_count_of<Ty>();
}

/**
 * @brief Get the name of a member.
 *
 */
template <size_t Index, reflectible Ty>
[[nodiscard]] consteval auto name_of() noexcept {
    constexpr auto count = member_count_of<Ty>();
    static_assert(Index < count, "Index out of range");
    constexpr auto names = member_names_of<Ty>();
    return names[Index];
}

/**
 * @brief Get the name of a member.
 *
 */
template <reflectible Ty>
[[nodiscard]] constexpr auto name_of(size_t index) noexcept {
    constexpr auto names = member_names_of<Ty>();
    return names[index];
}

template <tstring Name, typename Ty>
constexpr auto& get_by_name(Ty& obj) noexcept {
    constexpr auto index = index_of<Name, Ty>();
    static_assert(index < _reflection::member_count_of<Ty>());
    return _reflection::get<index>(obj);
}

} // namespace neutron
