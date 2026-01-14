// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/erase.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/same.hpp"

namespace neutron {

template <typename TypeList>
struct type_list_combine;
template <template <typename...> typename Template>
struct type_list_combine<Template<>> {
    using type = Template<>;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_combine<Template<Ty, Others...>> {
    template <typename T>
    using _has_same_template = type_list_has_same_template<Ty, T>;
    using type               = type_list_cat_t<
                      type_list_list_cat_t<
                          type_list_filt_t<_has_same_template, Template<Ty, Others...>>>,
                      typename type_list_combine<type_list_erase_if_t<
                          _has_same_template, Template<Ty, Others...>>>::type>;
};
template <typename TypeList>
using type_list_conbine_t = typename type_list_combine<TypeList>::type;
} // namespace neutron
