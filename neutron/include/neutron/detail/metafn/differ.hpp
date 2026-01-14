// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/has.hpp"

namespace neutron {

template <typename TypeList1, typename TypeList2>
struct type_list_all_differs_from;
template <
    template <typename...> typename Template1,
    template <typename...> typename Template2, typename... Tys1,
    typename... Tys2>
struct type_list_all_differs_from<Template1<Tys1...>, Template2<Tys2...>> {
    constexpr static bool value =
        !(type_list_has_v<Tys1, Template2<Tys2...>> || ...);
};
template <typename TypeList1, typename TypeList2>
constexpr auto type_list_all_differs_from_v =
    type_list_all_differs_from<TypeList1, TypeList2>::value;

} // namespace neutron
