#pragma once
#include "neutron/detail/reflection/hash.hpp"
#include "neutron/detail/reflection/meta/name_of.hpp"

namespace neutron {

template <typename Ty, auto Hasher = ::neutron::internal::hash>
requires std::is_same_v<Ty, std::remove_cvref_t<Ty>>
consteval auto hash_of() noexcept {
    constexpr auto name = name_of<Ty>();
    return Hasher(name);
}

} // namespace neutron
