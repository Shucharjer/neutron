// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

// type_list_first

ATOM_EXPORT template <typename>
struct type_list_first;
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_first<Template<Ty, Others...>> {
    using type = Ty;
};
ATOM_EXPORT template <typename Tl>
using type_list_first_t = typename type_list_first<Tl>::type;

// type_list_last

ATOM_EXPORT template <typename>
struct type_list_last;
template <template <typename...> typename Template, typename Ty>
struct type_list_last<Template<Ty>> {
    using type = Ty;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_last<Template<Ty, Others...>> {
    using type = typename type_list_last<Template<Others...>>::type;
};
ATOM_EXPORT template <typename Tl>
using type_list_last_t = typename type_list_last<Tl>::type;

} // namespace neutron
