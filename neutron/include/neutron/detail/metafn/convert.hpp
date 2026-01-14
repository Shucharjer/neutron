// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once

namespace neutron {

template <template <typename> typename Predicate, typename>
struct type_list_convert;
template <
    template <typename> typename Predicate,
    template <typename...> typename Template, typename... Tys>
struct type_list_convert<Predicate, Template<Tys...>> {
    using type = Template<typename Predicate<Tys>::type...>;
};
template <template <typename> typename Predicate, typename TypeList>
using type_list_convert_t =
    typename type_list_convert<Predicate, TypeList>::type;

}