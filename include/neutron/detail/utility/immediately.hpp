// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT struct immediately_t {};

ATOM_EXPORT constexpr inline immediately_t immediately{};

} // namespace neutron
