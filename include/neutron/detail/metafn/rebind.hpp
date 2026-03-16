// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once

namespace neutron {

template <template <typename...> typename Template, typename TypeList>
struct type_list_rebind;
template <
    template <typename...> typename Template,
    template <typename...> typename Tmp, typename... Tys>
struct type_list_rebind<Template, Tmp<Tys...>> {
    using type = Template<Tys...>;
};
template <template <typename...> typename Template, typename TypeList>
using type_list_rebind_t = typename type_list_rebind<Template, TypeList>::type;

} // namespace neutron
