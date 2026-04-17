#pragma once
#include <cstdint>
#include <string_view>
#include "neutron/detail/reflection/hash_t.hpp"
#include "neutron/detail/reflection/refl.hpp"
#include "neutron/detail/reflection/type_traits.hpp"

namespace neutron {

class member_info {
    friend class type_info;

public:
    ATOM_NODISCARD constexpr std::string_view name() const noexcept {
        return name_;
    }

    ATOM_NODISCARD constexpr type_traits traits() const noexcept {
        return traits_;
    }

private:
    type_traits traits_;
    std::string_view name_;
};

class type_info {
    template <typename T>
    friend consteval type_info type_info_of() noexcept;

    consteval type_info(
        std::uint32_t hash, type_traits traits, std::string_view name) noexcept
        : hash_(hash), traits_(traits), name_(name) {}

public:
    ATOM_NODISCARD constexpr std::string_view name() const noexcept {
        return name_;
    }

    ATOM_NODISCARD constexpr std::size_t hash() const noexcept { return hash_; }

    ATOM_NODISCARD constexpr std::strong_ordering
        operator<=>(const type_info& that) const noexcept {
        auto order = hash_ <=> that.hash_;
        return order != std::strong_ordering::equal ? order
                                                    : name_ <=> that.name_;
    }

    ATOM_NODISCARD constexpr bool
        operator==(const type_info& that) const noexcept {
        return hash_ == that.hash_ && name_ == that.name_;
    }

private:
    hash_t hash_;
    type_traits traits_;
    std::string_view name_;
};

template <typename T>
consteval type_info type_info_of() noexcept {
    return type_info{ hash_of<T>(), type_traits_of<T>(), name_of<T>() };
}

} // namespace neutron
