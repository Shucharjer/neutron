// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <tuple>
#include <utility>

namespace neutron {
template <typename Ty, typename Tuple>
struct tuple_first;
template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct tuple_first<Ty, Template<Types...>> {
    constexpr static size_t value =
        []<size_t... Is>(std::index_sequence<Is...>) {
            auto index = static_cast<size_t>(-1);
            (..., (index = (std::is_same_v<Ty, Types> ? Is : index)));
            return index;
        }(std::index_sequence_for<Types...>());
    static_assert(value < sizeof...(Types), "Type not found in tuple");
};
template <typename Ty, typename Tuple>
constexpr size_t tuple_first_v = tuple_first<Ty, Tuple>::value;

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_first(std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_first_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_first(const std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_first_v<Ty, std::tuple<Tys...>>>(tuple);
}



} // namespace neutron
