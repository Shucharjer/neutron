// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/erase.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/first_last.hpp"
#include "neutron/detail/metafn/pop.hpp"

namespace neutron {

template <template <typename, typename> typename Predicate, typename TypeList>
struct type_list_sort;
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template>
struct type_list_sort<Predicate, Template<>> {
    using type = Template<>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename Ty>
struct type_list_sort<Predicate, Template<Ty>> {
    using type = Template<Ty>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename Lhs, typename Rhs>
struct type_list_sort<Predicate, Template<Lhs, Rhs>> {
    using type = std::conditional_t<
        Predicate<Lhs, Rhs>::value, Template<Lhs, Rhs>, Template<Rhs, Lhs>>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename... Tys>
struct type_list_sort<Predicate, Template<Tys...>> {
    using pivot_t     = type_list_first_t<Template<Tys...>>;
    using pop_first_t = type_list_pop_first_t<Template<Tys...>>;

    template <typename Ty>
    struct comp {
        constexpr static bool value = Predicate<Ty, pivot_t>::value;
    };

    using left = typename type_list_sort<
        Predicate, type_list_filt_t<comp, pop_first_t>>::type;
    using right = typename type_list_sort<
        Predicate, type_list_erase_in_t<left, pop_first_t>>::type;
    using type = type_list_cat_t<left, Template<pivot_t>, right>;
};
template <template <typename, typename> typename Predicate, typename TypeList>
using type_list_sort_t = typename type_list_sort<Predicate, TypeList>::type;

} // namespace neutron
