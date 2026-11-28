// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename...>
struct type_list {};

ATOM_EXPORT template <typename>
struct is_type_list : std::false_type {};
template <typename... Tparams>
struct is_type_list<type_list<Tparams...>> : std::true_type {};
ATOM_EXPORT template <typename Ty>
constexpr bool is_type_list_v = is_type_list<Ty>::value;

ATOM_EXPORT template <auto...>
struct value_list {};

ATOM_EXPORT template <typename>
struct is_value_list : std::false_type {};
template <auto... Tparams>
struct is_value_list<value_list<Tparams...>> : std::true_type {};
ATOM_EXPORT template <typename Ty>
constexpr bool is_value_list_v = is_value_list<Ty>::value;

ATOM_EXPORT template <typename... Args>
using decayed_type_list = type_list<std::decay_t<Args>...>;

ATOM_EXPORT template <typename Tag, typename... Args>
struct tagged_type_list {
    using tag       = Tag;
    using type_list = ::neutron::type_list<Args...>;
};

ATOM_EXPORT template <typename Tag, auto... Args>
struct tagged_value_list {
    using tag        = Tag;
    using value_list = ::neutron::value_list<Args...>;
};

} // namespace neutron
