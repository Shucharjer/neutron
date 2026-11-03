// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include "neutron/detail/utility/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <cstddef>
    #include <meta>
    #include <tuple>
    #include <utility>

namespace neutron {

namespace _refl_meta_offset {

template <typename Ty>
inline constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(
        ^^Ty, std::meta::access_context::unchecked()));

template <typename Ty, std::size_t Index>
using member_pointer_t =
    typename[:std::meta::type_of(members<Ty>[Index]):] Ty::*;

} // namespace _refl_meta_offset

/**
 * @brief Get the pointer to the Index-th non-static data member of Ty.
 */
template <std::size_t Index, typename Ty>
requires(Index < _refl_meta_offset::members<Ty>.size())
ATOM_NODISCARD consteval auto offset_of() noexcept {
    using pointer_t = _refl_meta_offset::member_pointer_t<Ty, Index>;
    return std::meta::extract<pointer_t>(_refl_meta_offset::members<Ty>[Index]);
}

/**
 * @brief Get the pointers to all non-static data members of Ty as a tuple.
 */
template <typename Ty>
ATOM_NODISCARD consteval auto offsets_of() noexcept {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(offset_of<Is, Ty>()...);
    }(std::make_index_sequence<_refl_meta_offset::members<Ty>.size()>());
}

/**
 * @brief Get the byte offset of the Index-th non-static data member of Ty.
 */
template <std::size_t Index, typename Ty>
requires(Index < _refl_meta_offset::members<Ty>.size())
ATOM_NODISCARD consteval std::size_t offset_value_of() noexcept {
    return std::meta::offset_of(_refl_meta_offset::members<Ty>[Index]).bytes;
}

} // namespace neutron

#endif
