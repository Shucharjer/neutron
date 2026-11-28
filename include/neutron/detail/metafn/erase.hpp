// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/metafn/filter.hpp"
#include "neutron/detail/metafn/has.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

// type_list_erase_if

ATOM_EXPORT template <template <typename> typename Predicate, typename TypeList>
struct type_list_erase_if {
    template <typename Ty>
    using _predicate_type = std::negation<Predicate<Ty>>;
    using type            = type_list_filter_t<_predicate_type, TypeList>;
};
ATOM_EXPORT template <template <typename> typename Predicate, typename TypeList>
using type_list_erase_if_t =
    typename type_list_erase_if<Predicate, TypeList>::type;

// type_list_erase

ATOM_EXPORT template <typename TypeList, typename Ty>
struct type_list_erase {
    template <typename T>
    using _predicate_type = std::is_same<T, Ty>;
    using type            = type_list_filter_t<_predicate_type, TypeList>;
};
ATOM_EXPORT template <typename TypeList, typename Ty>
using type_list_erase_t = typename type_list_erase<TypeList, Ty>::type;

// type_list_erase_in

ATOM_EXPORT template <typename TypeList, typename List>
struct type_list_erase_in {
    template <typename T>
    using predicate_type = std::negation<type_list_has<List, T>>;
    using type           = type_list_filter_t<predicate_type, TypeList>;
};
ATOM_EXPORT template <typename TypeList, typename List>
using type_list_erase_in_t = typename type_list_erase_in<TypeList, List>::type;

} // namespace neutron
