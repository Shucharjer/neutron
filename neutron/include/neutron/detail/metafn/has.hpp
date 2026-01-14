#pragma once
#include <type_traits>
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

// type_list_has

template <typename, typename>
struct type_list_has : std::false_type {};

template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Ty, Template<Types...>> :
    std::bool_constant<(std::is_same_v<Ty, Types> || ...)> {};

template <typename TypeList, typename Ty>
constexpr bool type_list_has_v = type_list_has<TypeList, Ty>::value;

// value_list_has

template <auto Val, typename>
struct value_list_has : std::false_type {};

template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Val, Template<Vals...>> :
    std::bool_constant<(
        std::is_same_v<value_list<Val>, value_list<Vals>> || ...)> {};

template <auto Val, typename TypeList>
constexpr bool value_list_has_v = value_list_has<Val, TypeList>::value;

} // namespace neutron
