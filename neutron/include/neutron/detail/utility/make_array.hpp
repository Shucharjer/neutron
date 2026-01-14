// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <array>

namespace neutron {

template <typename Ty, typename... Args>
consteval auto make_array(Args&&... args) {
    return std::array<Ty, sizeof...(Args)>{ Ty{ std::forward<Args>(args) }... };
}

} // namespace neutron

using neutron::make_array;
