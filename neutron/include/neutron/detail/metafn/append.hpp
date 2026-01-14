// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once

namespace neutron {

template <template <typename...> typename List, typename>
struct append_type_list;
template <
    template <typename...> typename List,
    template <typename...> typename Template, typename... Tys>
struct append_type_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <typename...> typename List, typename Target>
using append_type_list_t = typename append_type_list<List, Target>::type;

template <template <auto...> typename List, typename>
struct append_value_list;
template <
    template <auto...> typename List, template <typename...> typename Template,
    typename... Tys>
struct append_value_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <auto...> typename List, typename Target>
using append_value_list_t = typename append_value_list<List, Target>::type;

} // namespace neutron
