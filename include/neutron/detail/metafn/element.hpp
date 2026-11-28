// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <std::size_t Index, typename TypeList>
struct type_list_element;

template <
    template <typename...> typename Template, typename Ty, typename... Args>
struct type_list_element<0, Template<Ty, Args...>> {
    using type = Ty;
};
template <
    std::size_t Index, template <typename...> typename Template, typename Ty,
    typename... Args>
struct type_list_element<Index, Template<Ty, Args...>> {
    using type = type_list_element<Index - 1, Template<Args...>>::type;
};
ATOM_EXPORT template <std::size_t Index, typename TypeList>
using type_list_element_t = typename type_list_element<Index, TypeList>::type;

ATOM_EXPORT template <std::size_t Index, typename Ty>
struct value_list_element;
template <template <auto...> typename Template, auto Val, auto... Others>
struct value_list_element<0, Template<Val, Others...>> {
    constexpr static auto value = Val;
};
template <
    std::size_t Index, auto Val, auto... Others,
    template <auto...> typename Template>
struct value_list_element<Index, Template<Val, Others...>> {
    constexpr static auto value =
        value_list_element<Index - 1, Template<Others...>>::value;
};
ATOM_EXPORT template <std::size_t Index, typename Ty>
constexpr inline auto value_list_element_v =
    value_list_element<Index, Ty>::value;

} // namespace neutron
