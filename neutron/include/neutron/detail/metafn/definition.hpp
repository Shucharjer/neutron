// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <typename...>
struct type_list {};

template <typename>
struct is_type_list : std::false_type {};
template <typename... Tparams>
struct is_type_list<type_list<Tparams...>> : std::true_type{};
template <typename Ty>
constexpr bool is_type_list_v = is_type_list<Ty>::value;

template <auto...>
struct value_list {};

template <typename>
struct is_value_list : std::false_type {};
template <auto... Tparams>
struct is_value_list<value_list<Tparams...>> : std::true_type {};
template <typename Ty>
constexpr bool is_value_list_v = is_value_list<Ty>::value;

} // namespace neutron
