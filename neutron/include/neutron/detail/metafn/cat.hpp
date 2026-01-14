// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once

namespace neutron {

// type_list_cat<...>

template <typename... Tls>
struct type_list_cat;
template <template <typename...> typename Template, typename... Tys>
struct type_list_cat<Template<Tys...>> {
    using type = Template<Tys...>;
};
template <
    template <typename...> typename Template, typename... Tys1,
    typename... Tys2>
struct type_list_cat<Template<Tys1...>, Template<Tys2...>> {
    using type = Template<Tys1..., Tys2...>;
};
template <typename Tl1, typename Tl2, typename... Tls>
struct type_list_cat<Tl1, Tl2, Tls...> {
    using type = typename type_list_cat<
        typename type_list_cat<Tl1, Tl2>::type, Tls...>::type;
};
template <typename... Tls>
using type_list_cat_t = typename type_list_cat<Tls...>::type;

// value_list_cat<...>

template <typename...>
struct value_list_cat;
template <template <auto...> typename Template>
struct value_list_cat<Template<>> {
    using type = Template<>;
};
template <template <auto...> typename Template, auto... Vals>
struct value_list_cat<Template<Vals...>> {
    using type = Template<Vals...>;
};
template <template <auto...> typename Template, auto... Vals1, auto... Vals2>
struct value_list_cat<Template<Vals1...>, Template<Vals2...>> {
    using type = Template<Vals1..., Vals2...>;
};
template <typename Vl1, typename Vl2, typename... Vls>
struct value_list_cat<Vl1, Vl2, Vls...> {
    using type = typename value_list_cat<
        typename value_list_cat<Vl1, Vl2>::type, Vls...>::type;
};
template <typename... Vls>
using value_list_cat_t = typename value_list_cat<Vls...>::type;

// type_list_list_cat<...>

template <typename>
struct type_list_list_cat;
template <template <typename...> typename Template>
struct type_list_list_cat<Template<>> {
    using type = Template<>;
};
template <template <typename...> typename Template, typename... Lists>
struct type_list_list_cat<Template<Lists...>> {
    using type = Template<type_list_cat_t<Lists...>>;
};
template <typename TypeList>
using type_list_list_cat_t = typename type_list_list_cat<TypeList>::type;

// type_list_value_list_cat<...>

template <typename TypeList>
struct type_list_value_list_cat;
template <template <typename...> typename Template, typename... ValueLists>
struct type_list_value_list_cat<Template<ValueLists...>> {
    using type = Template<value_list_cat_t<ValueLists...>>;
};
template <typename TypeList>
using type_list_value_list_cat_t =
    typename type_list_value_list_cat<TypeList>::type;

} // namespace neutron
