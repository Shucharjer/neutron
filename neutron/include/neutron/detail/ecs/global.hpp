#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace neutron {

template <typename... Args>
struct global : public std::tuple<Args...> {};

namespace internal {

template <typename>
struct _is_global : std::false_type {};

template <typename... Args>
struct _is_global<global<Args...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Args>
struct std::tuple_size<neutron::global<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::global<Args...>> {
    using type = std::tuple_element_t<Index, std::tuple<Args...>>;
};
