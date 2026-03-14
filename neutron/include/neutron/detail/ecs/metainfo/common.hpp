#pragma once
#include <concepts>
#include <type_traits>
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/first_last.hpp"

namespace neutron {

template <typename Tag, typename TypeList>
struct fetch_vinfo {
    using type = type_list_first_t<type_list_filt_tagged_nempty_t<
        Tag, std::remove_cvref_t<TypeList>, type_list<tagged_value_list<Tag>>>>;
};

template <typename Tag, typename TypeList>
using fetch_vinfo_t = typename fetch_vinfo<Tag, TypeList>::type;

template <typename Tag, typename TypeList>
struct fetch_tinfo {
    using type = type_list_first_t<type_list_filt_tagged_nempty_t<
        Tag, std::remove_cvref_t<TypeList>, type_list<tagged_type_list<Tag>>>>;
};

template <typename Tag, typename TypeList>
using fetch_tinfo_t = typename fetch_tinfo<Tag, TypeList>::type;

template <typename T>
struct writable : std::false_type {};

template <typename T>
struct writable<T&> : std::bool_constant<!std::is_const_v<T>> {};

template <typename T>
struct readonly : std::negation<writable<T>> {};

template <typename T, bool Readonly>
struct _accessibility_t {
    using type = T;
};

template <typename T>
using accessibility = _accessibility_t<T, readonly<T>::value>;

} // namespace neutron
