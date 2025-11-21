#pragma once
#include <type_traits>
#include <utility>
#include "pair.hpp"

namespace neutron {

template <typename Template, typename Object>
struct rebind_template;
template <template <typename> typename Template, typename Void, typename Object>
struct rebind_template<Template<Void>, Object> {
    using type = Template<Object>;
};
template <typename Template, typename Object>
using rebind_template_t = typename rebind_template<Template, Object>::type;

template <typename Ty>
concept _adaptor_closure_object =
    std::is_base_of_v<std::remove_cvref_t<Ty>, std::remove_cvref_t<Ty>>;

template <typename Ty>
concept strict_pipeline = requires {
    typename std::remove_cvref_t<Ty>::input_require;
    typename std::remove_cvref_t<Ty>::output_type;
    typename rebind_template<
        typename std::remove_cvref_t<Ty>::input_require, void>::type;
};
template <typename Ty>
concept consteval_strict_pipeline = strict_pipeline<Ty> && requires {
    typename std::remove_cvref_t<Ty>::constval_pipe;
};

template <typename Derived>
struct adaptor_closure {
    friend Derived;
    constexpr adaptor_closure() noexcept = default;
};

template <typename First, typename Second>
class _closure_compose :
    public adaptor_closure<_closure_compose<First, Second>> {
public:
    template <typename Left, typename Right>
    constexpr _closure_compose(Left&& left, Right&& right) noexcept(
        std::is_nothrow_constructible_v<
            compressed_pair<First, Second>, Left, Right>)
        : closures_(std::forward<Left>(left), std::forward<Right>(right)) {}

    constexpr ~_closure_compose() noexcept(
        std::is_nothrow_destructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr _closure_compose(const _closure_compose&) noexcept(
        std::is_nothrow_copy_constructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr _closure_compose(_closure_compose&& that) noexcept(
        std::is_nothrow_move_constructible_v<compressed_pair<First, Second>>)
        : closures_(std::move(that.closures_)) {}

    constexpr _closure_compose& operator=(const _closure_compose&) noexcept(
        std::is_nothrow_copy_assignable_v<compressed_pair<First, Second>>) =
        default;

    constexpr _closure_compose& operator=(_closure_compose&& that) noexcept(
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

template <typename First, typename Second>
_closure_compose(First&&, Second&&) -> _closure_compose<
    std::remove_cvref_t<First>, std::remove_cvref_t<Second>>;

template <typename First, typename Second>
struct closure_compose : public _closure_compose<First, Second> {
    using input_require = typename std::remove_cvref_t<First>::input_require;
    using output_type   = typename std::remove_cvref_t<Second>::output_type;
    using _compose_base = _closure_compose<First, Second>;
    using _compose_base::_closure_compose;
    using _compose_base::operator=;
    using _compose_base::operator();
};

} // namespace neutron

template <typename Ty, neutron::strict_pipeline Closure>
requires neutron::rebind_template_t<
    typename std::remove_cvref_t<Closure>::input_require, Ty>::value
constexpr decltype(auto) operator|(Ty&& object, Closure&& closure) {
    return std::forward<Closure>(closure)(std::forward<Ty>(object));
}

template <neutron::strict_pipeline Closure1, neutron::strict_pipeline Closure2>
requires neutron::rebind_template_t<
    typename std::remove_cvref_t<Closure2>::input_require,
    typename std::remove_cvref_t<Closure1>::output_type>::value
constexpr decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return neutron::closure_compose<Closure1, Closure2>{
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2)
    };
}

template <typename Ty, neutron::consteval_strict_pipeline Closure>
requires neutron::rebind_template_t<
    typename std::remove_cvref_t<Closure>::input_require, Ty>::value
consteval decltype(auto) operator|(Ty&& object, Closure&& closure) {
    return std::forward<Closure>(closure)(std::forward<Ty>(object));
}

template <
    neutron::strict_pipeline Closure1,
    neutron::consteval_strict_pipeline Closure2>
requires neutron::rebind_template_t<
    typename std::remove_cvref_t<Closure2>::input_require,
    typename std::remove_cvref_t<Closure1>::output_type>::value
consteval decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return neutron::closure_compose<Closure1, Closure2>{
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2)
    };
}
