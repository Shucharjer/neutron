// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/definition.hpp"

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

template <template <auto> typename Predicate, typename>
struct value_list_convert;
template <
    template <auto> typename Predicate, template <auto...> typename Template,
    auto... Vals>
struct value_list_convert<Predicate, Template<Vals...>> {
    using type = Template<Predicate<Vals>::type...>;
};
template <template <auto> typename Predicate, typename ValueList>
using value_list_convert_t =
    typename value_list_convert<Predicate, ValueList>::type;

template <
    template <auto> typename Predicate, typename ValueList,
    template <typename...> typename Template = type_list>
struct value_list_to_type_list_convert;
template <
    template <auto> typename Predicate, template <auto...> typename Tmp,
    auto... Vals, template <typename...> typename Template>
struct value_list_to_type_list_convert<Predicate, Tmp<Vals...>, Template> {
    using type = Template<typename Predicate<Vals>::type...>;
};
template <
    template <auto> typename Predicate, typename ValueList,
    template <typename...> typename Template = type_list>
using value_list_to_type_list_convert_t =
    typename value_list_to_type_list_convert<
        Predicate, ValueList, Template>::type;

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
