#pragma once
#include <cstddef>
#include <tuple>
#include "./concepts.hpp"
#include "./tuple_view.hpp"

namespace neutron {

namespace _reflection {

template <std::size_t Index, reflectible Ty>
struct member_type_of;

template <std::size_t Index, reflectible Ty>
requires default_reflectible_aggregate<Ty>
struct member_type_of<Index, Ty> {
    using type = std::remove_pointer_t<
        std::tuple_element_t<Index, decltype(struct_to_tuple_view<Ty>())>>;
};

template <std::size_t Index, reflectible Ty>
requires has_field_traits<Ty>
struct member_type_of<Index, Ty> {
    using type = typename std::tuple_element_t<
        Index, decltype(Ty::field_traits())>::type;
};

template <std::size_t Index, reflectible Ty>
using member_type_of_t = typename member_type_of<Index, Ty>::type;

} // namespace _reflection

using _reflection::member_type_of;
using _reflection::member_type_of_t;

} // namespace neutron
