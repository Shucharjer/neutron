#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

// type_list_has

ATOM_EXPORT template <typename, typename>
struct type_list_has : std::false_type {};

template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Template<Types...>, Ty> :
    std::bool_constant<(std::is_same_v<Ty, Types> || ...)> {};

ATOM_EXPORT template <typename TypeList, typename Ty>
constexpr bool type_list_has_v = type_list_has<TypeList, Ty>::value;

// value_list_has

ATOM_EXPORT template <typename, auto Val>
struct value_list_has : std::false_type {};

template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Template<Vals...>, Val> :
    std::bool_constant<(
        std::is_same_v<value_list<Val>, value_list<Vals>> || ...)> {};

ATOM_EXPORT template <typename TypeList, auto Val>
constexpr bool value_list_has_v = value_list_has<TypeList, Val>::value;

ATOM_EXPORT template <typename Tl, typename Ty>
struct tagged_type_list_has : std::false_type {};
template <
    typename Ty, template <typename, typename...> typename Template,
    typename Tag, typename... Args>
struct tagged_type_list_has<Template<Tag, Args...>, Ty> :
    type_list_has<Template<Tag, Args...>, Ty> {};
ATOM_EXPORT template <typename Tl, typename Ty>
constexpr bool tagged_type_list_has_v = type_list_has<Tl, Ty>::value;

ATOM_EXPORT template <typename TaggedList, typename Tag>
struct tagged_type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Args>
struct tagged_type_list_has_tag<Template<Tag, Args...>, Tag> :
    std::true_type {};
ATOM_EXPORT template <typename TaggedList, typename Tag>
constexpr bool tagged_type_list_has_tag_v =
    tagged_type_list_has_tag<TaggedList, Tag>::value;

ATOM_EXPORT template <typename TaggedList, typename Tag>
struct tagged_value_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename, auto...> typename Template, auto... Args>
struct tagged_value_list_has_tag<Template<Tag, Args...>, Tag> :
    std::true_type {};
ATOM_EXPORT template <typename TaggedList, typename Tag>
constexpr bool tagged_value_list_has_tag_v =
    tagged_value_list_has_tag<TaggedList, Tag>::value;

ATOM_EXPORT template <typename TaggedList, typename Tag>
struct tagged_list_has_tag {
    static constexpr bool value = tagged_type_list_has_tag_v<TaggedList, Tag> ||
                                  tagged_value_list_has_tag_v<TaggedList, Tag>;
};
ATOM_EXPORT template <typename TaggedList, typename Tag>
constexpr bool tagged_list_has_tag_v =
    tagged_list_has_tag<TaggedList, Tag>::value;

ATOM_EXPORT template <typename TypeList, typename Tag>
struct type_list_has_tag : std::false_type {};
template <
    typename Tag, template <typename...> typename Template, typename... Tls>
struct type_list_has_tag<Template<Tls...>, Tag> {
    static constexpr bool value =
        (false || ... || tagged_type_list_has_tag_v<Tls, Tag>) ||
        (false || ... || tagged_value_list_has_tag_v<Tls, Tag>);
};
ATOM_EXPORT template <typename TypeList, typename Tag>
constexpr bool type_list_has_tag_v = type_list_has_tag<TypeList, Tag>::value;

} // namespace neutron
