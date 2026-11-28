// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename T, typename... U>
concept excluding = (!std::same_as<std::remove_cvref_t<T>, U> && ...);
}
