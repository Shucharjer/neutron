// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <concepts>

namespace neutron {

template <typename Ty, typename... Args>
concept one_of = (std::same_as<Ty, Args> || ...);

}