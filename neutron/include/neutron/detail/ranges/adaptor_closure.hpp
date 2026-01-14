// IWYU pragma: private, include <neutron/ranges.hpp>
#pragma once
#include <ranges>
#include <type_traits>

#if __cplusplus > 202002L

namespace neutron {

template <typename Derived>
using range_adaptor_closure = std::ranges::range_adaptor_closure<Derived>;

} // namespace neutron

#else

    #include <utility>
    #include "../pipeline.hpp"

namespace neutron {

template <typename Derived>
struct range_adaptor_closure : adaptor_closure<Derived> {};

/**
 * @brief The custom result of pipeline operator between two closures. It's a
 * special closure.
 *
 * @tparam First The type of the first range closure.
 * @tparam Second The type of the second range closure.
 */
template <typename First, typename Second>
struct _range_closure_compose :
    _closure_compose<
        _range_closure_compose, range_adaptor_closure, First, Second> {
    using _compose_base = _closure_compose<
        _range_closure_compose, range_adaptor_closure, First, Second>;
    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
_range_closure_compose(C1&&, C2&&)
    -> _range_closure_compose<std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <typename Closure>
concept _range_adaptor_closure =
    _adaptor_closure<Closure, range_adaptor_closure>;

template <typename Closure, typename Rng>
concept _range_adaptor_closure_for =
    std::ranges::range<Rng> && _range_adaptor_closure<Closure> &&
    std::ranges::range<std::invoke_result_t<Closure, Rng>>;

template <std::ranges::range Rng, _range_adaptor_closure_for<Rng> Closure>
constexpr decltype(auto) operator|(Rng&& range, Closure&& closure) noexcept(
    noexcept(std::forward<Closure>(closure)(std::forward<Rng>(range)))) {
    return std::forward<Closure>(closure)(std::forward<Rng>(range));
}

template <_range_adaptor_closure Closure1, _range_adaptor_closure Closure2>
constexpr decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return neutron::_range_closure_compose(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

template <typename WildClosure, _range_adaptor_closure Closure>
constexpr decltype(auto) operator|(WildClosure&& wild, Closure&& closure) {
    return neutron::_range_closure_compose<WildClosure, Closure>(
        std::forward<WildClosure>(wild), std::forward<Closure>(closure));
}

template <_range_adaptor_closure Closure, typename WildClosure>
constexpr decltype(auto) operator|(Closure&& closure, WildClosure&& wild) {
    return neutron::_range_closure_compose<
        std::remove_cvref_t<Closure>, WildClosure>(
        std::forward<Closure>(closure), std::forward<WildClosure>(wild));
}

} // namespace neutron

#endif
