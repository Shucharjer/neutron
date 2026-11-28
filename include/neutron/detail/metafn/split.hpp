// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <
    typename TypeList, template <typename...> typename Template = type_list>
struct type_list_split;
template <
    template <typename...> typename Tmp, typename... Tys,
    template <typename...> typename Template>
struct type_list_split<Tmp<Tys...>, Template> {
    using type = Tmp<Template<Tys>...>;
};
ATOM_EXPORT template <
    typename TypeList, template <typename...> typename Template = type_list>
using type_list_split_t = typename type_list_split<TypeList, Template>::type;

} // namespace neutron
