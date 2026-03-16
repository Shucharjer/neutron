// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/empty.hpp"
#include "neutron/detail/metafn/has.hpp"

namespace neutron {

template <typename TypeList, typename = empty_type_list_t<TypeList>>
struct unique_type_list;
template <template <typename...> typename Template, typename Prev>
struct unique_type_list<Template<>, Prev> {
    using type = Prev;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others,
    typename Prev>
struct unique_type_list<Template<Ty, Others...>, Prev> {
    using current_list = std::conditional_t<
        type_list_has<Ty, Prev>::value, Template<>, Template<Ty>>;
    using type = unique_type_list<
        Template<Others...>, type_list_cat_t<Prev, current_list>>::type;
};
template <typename Tl>
using unique_type_list_t = typename unique_type_list<Tl>::type;

template <typename ValueList, typename = empty_value_list<ValueList>>
struct unique_value_list;
template <
    template <auto...> typename Template, auto Val, auto... Others,
    typename Prev>
struct unique_value_list<Template<Val, Others...>, Prev> {
    using current_list = std::conditional_t<
        value_list_has<Val, Prev>::value, Template<>, Template<Val>>;
    using type = unique_value_list<
        Template<Others...>, type_list_cat_t<Prev, current_list>>::type;
};
template <typename Vl>
using unique_value_list_t = typename unique_value_list<Vl>::type;

} // namespace neutron
