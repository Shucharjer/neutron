// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Ty, typename TypeList>
struct type_list_has_same_template : std::false_type {};
template <
    template <typename...> typename Template, typename... Args1,
    typename... Args2>
struct type_list_has_same_template<Template<Args1...>, Template<Args2...>> :
    std::true_type {};
ATOM_EXPORT template <typename Ty, typename TypeList>
constexpr auto type_list_has_same_template_v =
    type_list_has_same_template<Ty, TypeList>::value;

} // namespace neutron
