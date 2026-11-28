#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

namespace _concepts {

template <typename T>
struct nonempty : std::negation<std::is_empty<T>> {};

} // namespace _concepts

ATOM_EXPORT template <typename T>
concept nonempty = _concepts::nonempty<T>::value;

} // namespace neutron
