// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/cat.hpp"

namespace neutron {

// type_list_export

template <template <typename...> typename Tmp, typename TypeList>
struct type_list_export;
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template>
struct type_list_export<Tmp, Template<>> {
    using type = Tmp<>;
};
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template, typename... Tys>
struct type_list_export<Tmp, Template<Tys...>> {
    template <typename Ty>
    struct expose {
        using type = Tmp<>;
    };
    template <typename... Args>
    struct expose<Tmp<Args...>> {
        using type = Tmp<Args...>;
    };
    using type = type_list_cat_t<typename expose<Tys>::type...>;
};
template <template <typename...> typename Tmp, typename TypeList>
using type_list_export_t = typename type_list_export<Tmp, TypeList>::type;

// value_list_export

template <template <auto...> typename Tmp, typename ValueTypeList>
struct value_list_export;
template <
    template <auto...> typename Tmp, template <typename...> typename Template>
struct value_list_export<Tmp, Template<>> {
    using type = Tmp<>;
};
template <
    template <auto...> typename Tmp, template <typename...> typename Template,
    typename... Tys>
struct value_list_export<Tmp, Template<Tys...>> {
    template <typename Ty>
    struct expose {
        using type = Tmp<>;
    };
    template <auto... Args>
    struct expose<Tmp<Args...>> {
        using type = Tmp<Args...>;
    };
    using type = value_list_cat_t<typename expose<Tys>::type...>;
};
template <template <auto...> typename Tmp, typename ValueTypeList>
using value_list_export_t =
    typename value_list_export<Tmp, ValueTypeList>::type;

} // namespace neutron
