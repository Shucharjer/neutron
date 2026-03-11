#pragma once
#include <type_traits>
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

// type_list_has

template <typename, typename>
struct type_list_has : std::false_type {};

template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Ty, Template<Types...>> :
    std::bool_constant<(std::is_same_v<Ty, Types> || ...)> {};

template <typename TypeList, typename Ty>
constexpr bool type_list_has_v = type_list_has<TypeList, Ty>::value;

// value_list_has

template <auto Val, typename>
struct value_list_has : std::false_type {};

template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Val, Template<Vals...>> :
    std::bool_constant<(
        std::is_same_v<value_list<Val>, value_list<Vals>> || ...)> {};

template <auto Val, typename TypeList>
constexpr bool value_list_has_v = value_list_has<Val, TypeList>::value;

template <typename Ty, typename Tl>
struct tagged_type_list_has : std::false_type {};
template <
    typename Ty, template <typename, typename...> typename Template,
    typename... Args>
struct tagged_type_list_has<Ty, Template<Args...>> :
    type_list_has<Ty, Template<Args...>> {};
template <typename Ty, typename Tl>
constexpr bool tagged_type_list_has_v = type_list_has<Ty, Tl>::value;

template <typename Tag, typename TaggedList>
struct tagged_type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Args>
struct tagged_type_list_has_tag<Tag, Template<Tag, Args...>> :
    std::true_type {};
template <typename Tag, typename TaggedList>
constexpr bool tagged_type_list_has_tag_v =
    tagged_type_list_has_tag<Tag, TaggedList>::value;

template <typename Tag, typename TaggedList>
struct tagged_value_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename, auto...> typename Template, auto... Args>
struct tagged_value_list_has_tag<Tag, Template<Tag, Args...>> :
    std::true_type {};
template <typename Tag, typename TaggedList>
constexpr bool tagged_value_list_has_tag_v =
    tagged_value_list_has_tag<Tag, TaggedList>::value;

template <typename Tag, typename TaggedList>
struct tagged_list_has_tag {
    static constexpr bool value = tagged_type_list_has_tag_v<Tag, TaggedList> ||
                                  tagged_value_list_has_tag_v<Tag, TaggedList>;
};
template <typename Tag, typename TaggedList>
constexpr bool tagged_list_has_tag_v =
    tagged_list_has_tag<Tag, TaggedList>::value;

template <typename Tag, typename TypeList>
struct type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Tls>
struct type_list_has_tag<Tag, Template<Tls...>> {
    static constexpr bool value =
        (false || ... || tagged_type_list_has_tag_v<Tag, Tls>) ||
        (false || ... || tagged_value_list_has_tag_v<Tag, Tls>);
};
template <typename Tag, typename TypeList>
constexpr bool type_list_has_tag_v = type_list_has_tag<Tag, TypeList>::value;

} // namespace neutron
