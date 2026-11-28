// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <type_traits>
    #include <utility>
#endif
#include "neutron/detail/metafn/size.hpp"
#include "neutron/detail/utility/get.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

template <typename Ty, typename Tup>
constexpr std::size_t _rmcvref_first = static_cast<std::size_t>(-1);
template <
    typename Ty, template <typename...> typename Template, typename... Tys>
constexpr std::size_t _rmcvref_first<Ty, Template<Tys...>> =
    []<std::size_t... Is>(std::index_sequence<Is...>) {
        auto index = static_cast<std::size_t>(sizeof...(Tys));
        (...,
         (index = std::is_same_v<std::remove_cvref_t<Tys>, Ty> ? Is : index));
        return index;
    }(std::index_sequence_for<Tys...>());

ATOM_EXPORT template <typename Ty, typename Tup>
requires(
    _rmcvref_first<Ty, std::remove_cvref_t<Tup>> <
    type_list_size_v<std::remove_cvref_t<Tup>>)
constexpr decltype(auto) rmcvref_first(Tup&& tup) noexcept {
    using tuple          = std::remove_cvref_t<Tup>;
    constexpr auto index = _rmcvref_first<Ty, tuple>;
    return get<index>(std::forward<Tup>(tup));
}

} // namespace neutron
