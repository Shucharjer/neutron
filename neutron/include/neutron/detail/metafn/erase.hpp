// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/has.hpp"

namespace neutron {

// type_list_erase_if

template <template <typename> typename Predicate, typename TypeList>
struct type_list_erase_if {
    template <typename Ty>
    using _predicate_type = std::negation<Predicate<Ty>>;
    using type            = type_list_filt_t<_predicate_type, TypeList>;
};
template <template <typename> typename Predicate, typename TypeList>
using type_list_erase_if_t =
    typename type_list_erase_if<Predicate, TypeList>::type;

// type_list_erase

template <typename TypeList, typename Ty>
struct type_list_erase {
    template <typename T>
    using _predicate_type = std::is_same<T, Ty>;
    using type            = type_list_filt_t<_predicate_type, TypeList>;
};
template <typename TypeList, typename Ty>
using type_list_erase_t = typename type_list_erase<TypeList, Ty>::type;

// type_list_erase_in

template <typename TypeList, typename List>
struct type_list_erase_in {
    template <typename T>
    using predicate_type = std::negation<type_list_has<T, List>>;
    using type           = type_list_filt_t<predicate_type, TypeList>;
};
template <typename TypeList, typename List>
using type_list_erase_in_t = typename type_list_erase_in<TypeList, List>::type;

} // namespace neutron
