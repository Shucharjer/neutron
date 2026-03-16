// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <typename Ty, typename TypeList>
struct type_list_has_same_template : std::false_type {};
template <
    template <typename...> typename Template, typename... Args1,
    typename... Args2>
struct type_list_has_same_template<Template<Args1...>, Template<Args2...>> :
    std::true_type {};
template <typename Ty, typename TypeList>
constexpr auto type_list_has_same_template_v =
    type_list_has_same_template<Ty, TypeList>::value;

} // namespace neutron
