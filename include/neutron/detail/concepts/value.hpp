// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty>
concept value = std::is_same_v<std::remove_reference_t<Ty>, Ty>;

ATOM_EXPORT template <typename Ty>
concept lvalue_ref = std::is_lvalue_reference_v<Ty>;

ATOM_EXPORT template <typename Ty>
concept clvalue_ref = std::is_lvalue_reference_v<Ty> &&
                      std::is_const_v<std::remove_reference_t<Ty>>;

ATOM_EXPORT template <typename Ty>
concept rvalue_ref = std::is_rvalue_reference_v<Ty>;

} // namespace neutron
