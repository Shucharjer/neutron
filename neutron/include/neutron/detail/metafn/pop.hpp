// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once

namespace neutron {

template <typename TypeList>
struct type_list_pop_first;
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_pop_first<Template<Ty, Others...>> {
    using type = Template<Others...>;
};
template <typename TypeList>
using type_list_pop_first_t = typename type_list_pop_first<TypeList>::type;

} // namespace neutron
