#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <array>
    #include <cstddef>
#endif
#include "neutron/detail/metafn/size.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

namespace _make_array {

template <typename ValueList>
struct _make_helper;
template <typename Ty, Ty... Values>
struct _make_helper<std::integer_sequence<Ty, Values...>> {
    constexpr auto operator()() noexcept -> std::array<Ty, sizeof...(Values)> {
        return std::array<Ty, sizeof...(Values)>{ Values... };
    }
};
template <template <auto...> typename Template, typename Ty, Ty... Values>
struct _make_helper<Template<Values...>> {
    constexpr auto operator()() noexcept -> std::array<Ty, sizeof...(Values)> {
        return std::array<Ty, sizeof...(Values)>{ Values... };
    }
};

} // namespace _make_array

// Constrained, so that `make_array<Ty>()` of <neutron/utility.hpp> is chosen
// for an element type.
ATOM_EXPORT template <typename ValueList>
requires requires { _make_array::_make_helper<ValueList>{}(); }
constexpr auto make_array() {
    return _make_array::_make_helper<ValueList>{}();
}

} // namespace neutron
