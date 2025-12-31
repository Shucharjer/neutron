#pragma once
#include <type_traits>

namespace neutron {

template <typename Ty>
concept value = std::is_same_v<std::remove_reference_t<Ty>, Ty>;

template <typename Ty>
concept lvalue_ref = std::is_lvalue_reference_v<Ty>;

template <typename Ty>
concept clvalue_ref = std::is_lvalue_reference_v<Ty> &&
                      std::is_const_v<std::remove_reference_t<Ty>>;

template <typename Ty>
concept rvalue_ref = std::is_rvalue_reference_v<Ty>;

} // namespace neutron
