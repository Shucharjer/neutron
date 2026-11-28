// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <tuple>
#endif
#include "neutron/detail/reflection/legacy/concepts.hpp"
#include "neutron/detail/reflection/legacy/tuple_view.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron::_refl_legacy {

ATOM_EXPORT template <std::size_t Index, reflectible Ty>
struct member_type_of;

template <std::size_t Index, reflectible Ty>
requires default_reflectible_aggregate<Ty>
struct member_type_of<Index, Ty> {
    using type = std::remove_const_t<std::remove_pointer_t<
        std::tuple_element_t<Index, decltype(struct_to_tuple_view<Ty>())>>>;
};

template <std::size_t Index, reflectible Ty>
requires has_field_traits<Ty>
struct member_type_of<Index, Ty> {
    using type = typename std::tuple_element_t<
        Index, decltype(Ty::field_traits())>::type;
};

ATOM_EXPORT template <std::size_t Index, reflectible Ty>
using member_type_of_t = typename member_type_of<Index, Ty>::type;

} // namespace neutron::_refl_legacy
