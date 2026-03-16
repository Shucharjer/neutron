// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

// empty_type_list

template <typename>
struct empty_type_list;
template <template <typename...> typename Template, typename... Tys>
struct empty_type_list<Template<Tys...>> {
    using type = Template<>;
};

template <typename TypeList>
using empty_type_list_t = typename empty_type_list<TypeList>::type;

// empty_value_list

template <typename>
struct empty_value_list;
template <template <auto...> typename Template, auto... Values>
struct empty_value_list<Template<Values...>> {
    using type = Template<>;
};

template <typename ValueList>
using empty_value_list_t = typename empty_value_list<ValueList>::type;

// is_empty_emplate

template <typename Ty>
struct is_empty_template : std::false_type {};
template <template <typename...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <template <auto...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <typename Ty>
constexpr auto is_empty_template_v = is_empty_template<Ty>::value;

} // namespace neutron
