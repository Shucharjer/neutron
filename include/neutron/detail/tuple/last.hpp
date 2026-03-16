// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <tuple>
#include <utility>

namespace neutron {

template <typename Ty, typename Tuple>
struct tuple_last;
template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct tuple_last<Ty, Template<Types...>> {
    constexpr static size_t value =
        []<size_t... Is>(std::index_sequence<Is...>) {
            auto index = static_cast<size_t>(-1);
            ((index = (std::is_same_v<Ty, Types> ? Is : index)), ...);
            return index;
        }(std::index_sequence_for<Types...>());
    static_assert(value < sizeof...(Types), "Type not found in tuple");
};
template <typename Ty, typename Tuple>
constexpr size_t tuple_last_v = tuple_last<Ty, Tuple>::value;

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_last(std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_last(const std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_last(std::tuple<Tys...>&& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(std::move(tuple));
}

} // namespace neutron
