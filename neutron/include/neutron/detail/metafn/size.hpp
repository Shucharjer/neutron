// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <cstddef>

namespace neutron {

template <typename>
struct type_list_size;
template <template <typename...> typename Template, typename... Tys>
struct type_list_size<Template<Tys...>> {
    constexpr static size_t value = sizeof...(Tys);
};
template <typename Ty>
constexpr static auto type_list_size_v = type_list_size<Ty>::value;

template <typename Ty>
struct value_list_size;
template <template <auto...> typename Template, auto... Vals>
struct value_list_size<Template<Vals...>> {
    constexpr static std::size_t value = sizeof...(Vals);
};
template <typename Ty>
constexpr size_t value_list_size_v = value_list_size<Ty>::value;

} // namespace neutron
