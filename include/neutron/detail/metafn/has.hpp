#pragma once
#include <type_traits>
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

// type_list_has

template <typename, typename>
struct type_list_has : std::false_type {};

template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Template<Types...>, Ty> :
    std::bool_constant<(std::is_same_v<Ty, Types> || ...)> {};

template <typename TypeList, typename Ty>
constexpr bool type_list_has_v = type_list_has<TypeList, Ty>::value;

// value_list_has

template <typename, auto Val>
struct value_list_has : std::false_type {};

template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Template<Vals...>, Val> :
    std::bool_constant<(
        std::is_same_v<value_list<Val>, value_list<Vals>> || ...)> {};

template <typename TypeList, auto Val>
constexpr bool value_list_has_v = value_list_has<TypeList, Val>::value;

template <typename Tl, typename Ty>
struct tagged_type_list_has : std::false_type {};
template <
    typename Ty, template <typename, typename...> typename Template,
    typename Tag, typename... Args>
struct tagged_type_list_has<Template<Tag, Args...>, Ty> :
    type_list_has<Template<Tag, Args...>, Ty> {};
template <typename Tl, typename Ty>
constexpr bool tagged_type_list_has_v = type_list_has<Tl, Ty>::value;

template <typename TaggedList, typename Tag>
struct tagged_type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Args>
struct tagged_type_list_has_tag<Template<Tag, Args...>, Tag> :
    std::true_type {};
template <typename TaggedList, typename Tag>
constexpr bool tagged_type_list_has_tag_v =
    tagged_type_list_has_tag<TaggedList, Tag>::value;

template <typename TaggedList, typename Tag>
struct tagged_value_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename, auto...> typename Template, auto... Args>
struct tagged_value_list_has_tag<Template<Tag, Args...>, Tag> :
    std::true_type {};
template <typename TaggedList, typename Tag>
constexpr bool tagged_value_list_has_tag_v =
    tagged_value_list_has_tag<TaggedList, Tag>::value;

template <typename TaggedList, typename Tag>
struct tagged_list_has_tag {
    static constexpr bool value = tagged_type_list_has_tag_v<TaggedList, Tag> ||
                                  tagged_value_list_has_tag_v<TaggedList, Tag>;
};
template <typename TaggedList, typename Tag>
constexpr bool tagged_list_has_tag_v =
    tagged_list_has_tag<TaggedList, Tag>::value;

template <typename TypeList, typename Tag>
struct type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Tls>
struct type_list_has_tag<Template<Tls...>, Tag> {
    static constexpr bool value =
        (false || ... || tagged_type_list_has_tag_v<Tls, Tag>) ||
        (false || ... || tagged_value_list_has_tag_v<Tls, Tag>);
};
template <typename TypeList, typename Tag>
constexpr bool type_list_has_tag_v = type_list_has_tag<TypeList, Tag>::value;

} // namespace neutron
