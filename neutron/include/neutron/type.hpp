#pragma once
#include <type_traits>

namespace neutron ::concepts {

template <typename Ty>
concept value = std::is_same_v<std::remove_reference_t<Ty>, Ty>;

template <typename Ty>
concept ref = std::is_lvalue_reference_v<Ty>;

template <typename Ty>
concept cref = std::is_lvalue_reference_v<Ty> && std::is_const_v<std::remove_reference_t<Ty>>;

template <typename Ty>
concept rref = std::is_rvalue_reference_v<Ty>;

template <typename Ty>
concept map_like = requires {
    typename std::remove_cvref_t<Ty>::key_type;
    typename std::remove_cvref_t<Ty>::mapped_type;
};

} // namespace neutron::concepts
