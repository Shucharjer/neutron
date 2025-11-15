#pragma once

#if __cplusplus > 202002L

    #include <ranges>

namespace neutron {

template <typename Derived>
using range_adaptor_closure = std::ranges::range_adaptor_closure<Derived>;

template <typename Ty>
concept _range_adaptor_closure_object = std::is_base_of_v<
    range_adaptor_closure<std::remove_cvref_t<Ty>>, std::remove_cvref_t<Ty>>;

} // namespace neutron

#else

    #include <utility>
    #include "neutron/pair.hpp"

namespace neutron {

template <typename Derived>
struct range_adaptor_closure {
    friend Derived;

private:
    range_adaptor_closure() noexcept = default;
};

/**
 * @brief The custom result of pipeline operator between two closures. It's a
 * special closure.
 *
 * @tparam First The type of the first range closure.
 * @tparam Second The type of the second range closure.
 */
template <typename First, typename Second>
struct _range_pipeline_result :
    public range_adaptor_closure<_range_pipeline_result<First, Second>> {

    template <typename Left, typename Right>
    constexpr _range_pipeline_result(Left&& left, Right&& right) noexcept(
        std::is_nothrow_constructible_v<
            compressed_pair<First, Second>, Left, Right>)
        : closures_(std::forward<Left>(left), std::forward<Right>(right)) {}

    constexpr ~_range_pipeline_result() noexcept(
        std::is_nothrow_destructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr _range_pipeline_result(const _range_pipeline_result&) noexcept(
        std::is_nothrow_copy_constructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr _range_pipeline_result(_range_pipeline_result&& that) noexcept(
        std::is_nothrow_move_constructible_v<compressed_pair<First, Second>>)
        : closures_(std::move(that.closures_)) {}

    constexpr _range_pipeline_result&
        operator=(const _range_pipeline_result&) noexcept(
            std::is_nothrow_copy_assignable_v<compressed_pair<First, Second>>) =
            default;

    constexpr _range_pipeline_result&
        operator=(_range_pipeline_result&& that) noexcept(
            std::is_nothrow_move_assignable_v<compressed_pair<First, Second>>) {
        closures_ = std::move(that.closures_);
    }

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     * @tparam Arg This tparam could be deduced.
     */
    template <typename Arg>
    requires requires {
        std::forward<First>(std::declval<First>())(std::declval<Arg>());
        std::forward<Second>(std::declval<Second>())(
            std::forward<First>(std::declval<First>())(std::declval<Arg>()));
    }
    [[nodiscard]] constexpr auto operator()(Arg&& arg) noexcept(
        noexcept(std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg))))) {
        return std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg)));
    }

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     * @tparam Arg This tparam could be deduced.
     */
    template <typename Arg>
    requires requires {
        std::forward<First>(std::declval<First>())(std::declval<Arg>());
        std::forward<Second>(std::declval<Second>())(
            std::forward<First>(std::declval<First>())(std::declval<Arg>()));
    }
    [[nodiscard]] constexpr auto operator()(Arg&& arg) const
        noexcept(noexcept(std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg))))) {
        return std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg)));
    }

private:
    compressed_pair<First, Second> closures_;
};

template <typename Ty>
concept _range_adaptor_closure_object = std::is_base_of_v<
    range_adaptor_closure<std::remove_cvref_t<Ty>>, std::remove_cvref_t<Ty>>;

template <std::ranges::range Rng, typename Closure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(Rng&& range, Closure&& closure) noexcept(
    requires { std::forward<Closure>(closure)(std::forward<Rng>(range)); }) {
    return std::forward<Closure>(closure)(std::forward<Rng>(range));
}

template <typename Closure1, typename Closure2>
requires neutron::_range_adaptor_closure_object<Closure1> &&
         neutron::_range_adaptor_closure_object<Closure2>
constexpr decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return neutron::_range_pipeline_result<Closure1, Closure2>(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

template <typename WildClosure, typename Closure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(WildClosure&& wild, Closure&& closure) {
    return neutron::_range_pipeline_result<WildClosure, Closure>(
        std::forward<WildClosure>(wild), std::forward<Closure>(closure));
}

template <typename Closure, typename WildClosure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(Closure&& closure, WildClosure&& wild) {
    return neutron::_range_pipeline_result<Closure, WildClosure>(
        std::forward<Closure>(closure), std::forward<WildClosure>(wild));
}

} // namespace neutron

#endif
