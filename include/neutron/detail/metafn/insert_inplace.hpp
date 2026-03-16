// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <neutron/metafn.hpp>
#include "neutron/detail/metafn/append.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/empty.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/first_last.hpp"
#include "neutron/detail/metafn/has.hpp"
#include "neutron/detail/metafn/specific.hpp"
#include "neutron/detail/metafn/substitute.hpp"

namespace neutron {

namespace _metafn_insert_inplace {

template <
    template <typename...> typename Template, typename TypeList, typename Ty>
struct _type_list_impl;

template <
    template <typename...> typename Template,
    template <typename...> typename Tmp, typename... Tys, typename Ty>
struct _type_list_impl<Template, Tmp<Tys...>, Ty> {
    using type = Tmp<Tys..., Template<Ty>>;
};

template <
    template <typename...> typename Template,
    template <typename...> typename Tmp, typename... Tys, typename Ty>
requires(false || ... || is_specific_type_list_v<Template, Tys>)
struct _type_list_impl<Template, Tmp<Tys...>, Ty> {
    template <typename T>
    struct convert {
        using type = T;
    };
    template <typename T>
    requires is_specific_type_list_v<Template, T>
    struct convert<T> {
        using type = type_list_cat_t<T, Template<Ty>>;
    };
    using type = Tmp<typename convert<Tys>::type...>;
};

} // namespace _metafn_insert_inplace

template <
    template <typename...> typename Template, typename TypeList, typename Ty>
struct insert_type_list_inplace {
    using type = typename _metafn_insert_inplace::_type_list_impl<
        Template, Ty, TypeList>::type;
};
template <
    template <typename...> typename Template, typename Ty, typename Target>
using insert_type_list_inplace_t =
    typename insert_type_list_inplace<Template, Ty, Target>::type;

template <
    typename Tag, typename TypeList, typename Ty,
    template <typename, typename...> typename TaggedList = tagged_type_list>
struct insert_tagged_type_list_inplace;

namespace _metafn_insert_inplace {

template <
    typename Tag, template <typename...> typename Template, typename Ty,
    template <typename, typename...> typename TaggedList, typename... Tls>
struct _tagged_type_list_impl {
    using type = Template<Tls..., TaggedList<Tag, Ty>>;
};

template <
    typename Tag, template <typename...> typename Template, typename Ty,
    template <typename, typename...> typename TaggedList, typename... Tls>
requires type_list_has_tag_v<Tag, Template<Tls...>>
struct _tagged_type_list_impl<Tag, Template, Ty, TaggedList, Tls...> {
    template <typename T>
    struct convert {
        using type = T;
    };
    template <template <typename, typename...> typename Tmp, typename... Tys>
    struct convert<Tmp<Tag, Tys...>> {
        using type = Tmp<Tag, Tys..., Ty>;
    };
    using type = Template<typename convert<Tls>::type...>;
};

} // namespace _metafn_insert_inplace

template <
    typename Tag, template <typename...> typename Template, typename... Tls,
    typename Ty, template <typename, typename...> typename TaggedList>
struct insert_tagged_type_list_inplace<Tag, Template<Tls...>, Ty, TaggedList> {
    using type = typename _metafn_insert_inplace::_tagged_type_list_impl<
        Tag, Template, Ty, TaggedList, Tls...>::type;
};
template <
    typename Tag, typename TypeList, typename Ty,
    template <typename, typename...> typename TaggedList = tagged_type_list>
using insert_tagged_type_list_inplace_t =
    typename insert_tagged_type_list_inplace<
        Tag, TypeList, Ty, TaggedList>::type;

template <
    typename Tag, typename TypeList, auto Vals,
    template <typename, auto...> typename TaggedList = tagged_value_list>
struct insert_tagged_value_list_inplace;

namespace _metafn_insert_inplace {

template <
    typename Tag, template <typename...> typename Template, auto Val,
    template <typename, auto...> typename TaggedList, typename... Tls>
struct _tagged_value_list_impl {
    using type = Template<Tls..., TaggedList<Tag, Val>>;
};

template <
    typename Tag, template <typename...> typename Template, auto Val,
    template <typename, auto...> typename TaggedList, typename... Tls>
requires type_list_has_tag_v<Tag, Template<Tls...>>
struct _tagged_value_list_impl<Tag, Template, Val, TaggedList, Tls...> {
    template <typename T>
    struct convert {
        using type = T;
    };
    template <template <typename, auto...> typename Tmp, auto... Vals>
    struct convert<Tmp<Tag, Vals...>> {
        using type = Tmp<Tag, Vals..., Val>;
    };
    using type = Template<typename convert<Tls>::type...>;
};

} // namespace _metafn_insert_inplace

template <
    typename Tag, template <typename...> typename Template, typename... Tls,
    auto Val, template <typename, auto...> typename TaggedList>
struct insert_tagged_value_list_inplace<
    Tag, Template<Tls...>, Val, TaggedList> {
    using type = typename _metafn_insert_inplace::_tagged_value_list_impl<
        Tag, Template, Val, TaggedList, Tls...>::type;
};
template <
    typename Tag, typename TypeList, auto Val,
    template <typename, auto...> typename TaggedList = tagged_value_list>
using insert_tagged_value_list_inplace_t =
    typename insert_tagged_value_list_inplace<
        Tag, TypeList, Val, TaggedList>::type;

template <
    typename Tag, typename TypeList,
    template <typename, auto...> typename TaggedList, auto... Vals>
struct insert_range_tagged_value_list_inplace;

template <
    typename Tag, typename TypeList,
    template <typename, auto...> typename TaggedList>
struct insert_range_tagged_value_list_inplace<Tag, TypeList, TaggedList> {
    using type = TypeList;
};

template <
    typename Tag, typename TypeList,
    template <typename, auto...> typename TaggedList, auto Val>
struct insert_range_tagged_value_list_inplace<Tag, TypeList, TaggedList, Val> {
    using type =
        insert_tagged_value_list_inplace_t<Tag, TypeList, Val, TaggedList>;
};

template <
    typename Tag, typename TypeList,
    template <typename, auto...> typename TaggedList, auto Val, auto... Others>
struct insert_range_tagged_value_list_inplace<
    Tag, TypeList, TaggedList, Val, Others...> {
    using type = typename insert_range_tagged_value_list_inplace<
        Tag, insert_tagged_value_list_inplace_t<Tag, TypeList, Val, TaggedList>,
        TaggedList, Others...>::type;
};

template <
    typename Tag, typename TypeList,
    template <typename, auto...> typename TaggedList, auto... Vals>
using insert_range_tagged_value_list_inplace_t =
    typename insert_range_tagged_value_list_inplace<
        Tag, TypeList, TaggedList, Vals...>::type;

} // namespace neutron
