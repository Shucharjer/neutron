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

template <template <typename> typename Predicate, typename TaggedList>
struct tagged_list_convert;
template <
    template <typename> typename Predicate,
    template <typename, typename...> typename Template, typename Tag,
    typename... Args>
struct tagged_list_convert<Predicate, Template<Tag, Args...>> {
    using type = Template<Tag, typename Predicate<Args>::type...>;
};
template <template <typename> typename Predicate, typename TaggedList>
using tagged_list_convert_t =
    typename tagged_list_convert<Predicate, TaggedList>::type;

} // namespace neutron
