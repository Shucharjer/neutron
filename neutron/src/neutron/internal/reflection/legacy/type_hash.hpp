#pragma once
#include "./name.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */

namespace internal {

constexpr uint64_t hash(std::string_view string) noexcept {
    // fnv1a
    constexpr uint64_t magic = 0xcbf29ce484222325;
    constexpr uint64_t prime = 0x100000001b3;

    uint64_t value = magic;
    for (char ch : string) {
        value = value ^ ch;
        value *= prime;
    }

    return value;
}

} // namespace internal

/*! @endcond */

template <typename Ty, auto Hasher = internal::hash>
requires std::is_same_v<Ty, std::remove_cvref_t<Ty>>
consteval auto hash_of() noexcept {
    constexpr auto name = name_of<Ty>();
    return Hasher(name);
}

} // namespace neutron
