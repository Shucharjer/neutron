// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename>
struct type_list_size;
template <template <typename...> typename Template, typename... Tys>
struct type_list_size<Template<Tys...>> {
    constexpr static std::size_t value = sizeof...(Tys);
};
ATOM_EXPORT template <typename Ty>
constexpr auto type_list_size_v = type_list_size<Ty>::value;

ATOM_EXPORT template <typename Ty>
struct value_list_size;
template <template <auto...> typename Template, auto... Vals>
struct value_list_size<Template<Vals...>> {
    constexpr static std::size_t value = sizeof...(Vals);
};
ATOM_EXPORT template <typename Ty>
constexpr std::size_t value_list_size_v = value_list_size<Ty>::value;

} // namespace neutron
