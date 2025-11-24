#pragma once
#include <cstddef>
#include <tuple>
#include <utility>
#include "neutron/template_list.hpp"
#include "./get.hpp"

namespace neutron {

namespace _decompose {

constexpr inline struct decompose_t {
    /**
     * @brief Pre-calculate the index sequence for each length in sequence.
     * @return std::tuple<std::index_sequence<...>, ...>;
     */
    template <size_t... Ls>
    consteval static auto precal() noexcept {
        //
    }

    template <size_t... Ls, gettible<0> T>
    requires((Ls + ...) == std::tuple_size_v<std::remove_cvref_t<T>>)
    constexpr auto operator()(T&& tup) {
        using decompose_sequence = decltype(precal<Ls...>());
    }

    template <size_t... Ls, gettible<0> T>
    requires((Ls + ...) < std::tuple_size_v<std::remove_cvref_t<T>>)
    constexpr auto operator()(T&& tup) {
        using decompose_sequence = decltype(precal<Ls...>());
    }
} decompose;

} // namespace _decompose

using _decompose::decompose;

} // namespace neutron
