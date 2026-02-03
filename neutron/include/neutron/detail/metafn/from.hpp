// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <tuple>
#include <utility>
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/element.hpp"
#include "neutron/detail/metafn/size.hpp"

namespace neutron {

template <template <auto> typename, typename>
struct type_list_from_value;
template <
    template <auto> typename Predicate, template <auto...> typename Template,
    auto... Vals>
struct type_list_from_value<Predicate, Template<Vals...>> {
    using type = type_list<typename Predicate<Vals>::type...>;
};
template <template <auto> typename Predicate, typename ValList>
using type_list_from_value_t =
    typename type_list_from_value<Predicate, ValList>::type;
/**
 * @brief Make a tuple from a value_list.
 */
template <typename Ty>
requires is_value_list_v<Ty>
constexpr auto tuple_from_value() noexcept {
    return []<size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(value_list_element_v<Is, Ty>...);
    }(std::make_index_sequence<value_list_size_v<Ty>>());
}
/**
 * @brief Make a tuple from a value_list.
 */
template <typename Ty>
requires is_value_list_v<Ty>
constexpr auto tuple_from_value(Ty) noexcept {
    return []<size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(value_list_element_v<Is, Ty>...);
    }(std::make_index_sequence<value_list_size_v<Ty>>());
}

template <typename>
struct value_list_from;
template <typename Elem, Elem... Vals>
struct value_list_from<std::integer_sequence<Elem, Vals...>> {
    using type = value_list<Vals...>;
};
template <typename Ty>
using value_list_from_t = typename value_list_from<Ty>::type;

} // namespace neutron
