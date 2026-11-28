// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <tuple>
    #include <type_traits>
#endif
#include "neutron/detail/ecs/utility/systuple.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename... Args>
class local {
    using _tuple_t = std::tuple<Args...>;

public:
    static_assert((std::is_same_v<std::remove_const_t<Args>, Args> && ...));

    template <stage Stage, auto Sys>
    constexpr local(systuple<Stage, Sys, Args...>& tup) noexcept : tup_(tup) {}

    template <std::size_t Index>
    constexpr auto get() & noexcept -> std::tuple_element_t<Index, _tuple_t>& {
        return std::get<Index>(tup_);
    }

    template <std::size_t Index>
    constexpr auto get() const& noexcept
        -> const std::tuple_element_t<Index, _tuple_t>& {
        return std::get<Index>(tup_);
    }

private:
    _tuple_t& tup_; // NOLINT
};

} // namespace neutron

template <typename... Args>
struct std::tuple_size<neutron::local<Args...>> :
    std::integral_constant<std::size_t, sizeof...(Args)> {};

template <std::size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::local<Args...>> {
    using type = std::tuple_element_t<Index, std::tuple<Args&...>>;
};
