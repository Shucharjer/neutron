// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <template <typename...> typename Template, typename Ty>
struct is_specific_type_list : std::false_type {};
template <template <typename...> typename Template, typename... Tys>
struct is_specific_type_list<Template, Template<Tys...>> : std::true_type {};
template <template <typename...> typename Template, typename Ty>
constexpr auto is_specific_type_list_v =
    is_specific_type_list<Template, Ty>::value;

template <template <auto...> typename Template, typename>
struct is_specific_value_list : std::false_type {};
template <template <auto...> typename Template, auto... Vals>
struct is_specific_value_list<Template, Template<Vals...>> : std::true_type {};
template <template <auto...> typename Template, typename Ty>
constexpr bool is_specific_value_list_v =
    is_specific_value_list<Template, Ty>::value;

} // namespace neutron
