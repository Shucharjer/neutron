// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <tuple>
#include <type_traits>
#include "neutron/detail/ecs/concepts/stage.hpp"

namespace neutron {

template <stage Stage, auto Sys, typename... Args>
class systuple : public std::tuple<Args...> {
public:
    using tuple_type = std::tuple<Args...>;

    using std::tuple<Args...>::tuple;

    constexpr systuple() noexcept(
        std::is_nothrow_default_constructible_v<std::tuple<Args...>>) = default;

    constexpr systuple(const std::tuple<Args...>& tup) noexcept(
        std::is_nothrow_copy_constructible_v<tuple_type>)
        : tuple_type(tup) {}

    constexpr systuple(std::tuple<Args...>&& tup) noexcept(
        std::is_nothrow_move_constructible_v<tuple_type>)
        : tuple_type(std::move(tup)) {}
};

} // namespace neutron
