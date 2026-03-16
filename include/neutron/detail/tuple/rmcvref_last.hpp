// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>
#include <utility>
#include "neutron/detail/metafn/size.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron {

template <typename Ty, typename Tup>
constexpr size_t _rmcvref_last = static_cast<size_t>(-1);
template <
    typename Ty, template <typename...> typename Template, typename... Tys>
constexpr size_t _rmcvref_last<Ty, Template<Tys...>> =
    []<size_t... Is>(std::index_sequence<Is...>) {
        auto index = static_cast<size_t>(sizeof...(Tys));
        ((index = std::is_same_v<std::remove_cvref_t<Tys>, Ty> ? Is : index),
         ...);
        return index;
    }(std::index_sequence_for<Tys...>());

template <typename Ty, typename Tup>
requires(
    _rmcvref_last<Ty, std::remove_cvref_t<Tup>> <
    type_list_size_v<std::remove_cvref_t<Tup>>)
constexpr decltype(auto) rmcvref_last(Tup&& tup) noexcept {
    using tuple          = std::remove_cvref_t<Tup>;
    constexpr auto index = _rmcvref_last<Ty, tuple>;
    return get<index>(std::forward<Tup>(tup));
}

} // namespace neutron
