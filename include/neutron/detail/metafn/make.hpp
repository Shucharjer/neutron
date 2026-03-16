#pragma once
#include <array>
#include <cstddef>
#include <neutron/detail/metafn/size.hpp>

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

template <typename ValueList>
constexpr auto make_array() {
    return _make_array::_make_helper<ValueList>{}();
}

} // namespace neutron
