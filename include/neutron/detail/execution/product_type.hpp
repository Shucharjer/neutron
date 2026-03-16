#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace neutron::execution {

template <typename... Args>
struct _product_type : public std::tuple<Args...> {
    using std::tuple<Args...>::tuple;
};

template <typename... Args>
_product_type(Args&&...) -> _product_type<std::decay_t<Args>...>;

} // namespace neutron::execution

namespace std {

template <typename... Args>
struct tuple_size<::neutron::execution::_product_type<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct tuple_element<Index, ::neutron::execution::_product_type<Args...>> :
    std::tuple_element<Index, std::tuple<Args...>> {};

} // namespace std
