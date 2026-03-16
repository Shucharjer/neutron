// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <tuple>
#include <type_traits>

namespace neutron {

template <auto Sys, typename... Args>
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

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <auto Sys, typename>
struct _is_relevant_sys_tuple : std::false_type {};
template <auto Sys, typename... Args>
struct _is_relevant_sys_tuple<Sys, systuple<Sys, Args...>> : std::true_type {};

template <typename>
struct _is_empty_sys_tuple : std::false_type {};
template <auto Sys>
struct _is_empty_sys_tuple<systuple<Sys>> : std::true_type {};

} // namespace internal
/*! @endcond */

template <auto Sys, typename... Args>
struct to_systuple {
    using type = systuple<Sys, Args...>;
};
template <auto Sys, typename... Args>
using to_systuple_t = typename to_systuple<Sys, Args...>::type;

} // namespace neutron
