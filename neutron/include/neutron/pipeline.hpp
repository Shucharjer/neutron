#pragma once
#include <utility>
#include "neutron/pair.hpp"

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
class pipeline_result : public adaptor_closure<pipeline_result<First, Second>> {
public:
    using input_require = typename std::remove_cvref_t<First>::input_require;
    using output_type   = typename std::remove_cvref_t<Second>::output_type;

    template <typename Left, typename Right>
    constexpr pipeline_result(Left&& left, Right&& right) noexcept(
        std::is_nothrow_constructible_v<
            compressed_pair<First, Second>, Left, Right>)
        : closures_(std::forward<Left>(left), std::forward<Right>(right)) {}

    constexpr ~pipeline_result() noexcept(
        std::is_nothrow_destructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr pipeline_result(const pipeline_result&) noexcept(
        std::is_nothrow_copy_constructible_v<compressed_pair<First, Second>>) =
        default;

    constexpr pipeline_result(pipeline_result&& that) noexcept(
        std::is_nothrow_move_constructible_v<compressed_pair<First, Second>>)
        : closures_(std::move(that.closures_)) {}

    constexpr pipeline_result& operator=(const pipeline_result&) noexcept(
        std::is_nothrow_copy_assignable_v<compressed_pair<First, Second>>) =
        default;

    constexpr pipeline_result& operator=(pipeline_result&& that) noexcept(
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
    return neutron::pipeline_result<Closure1, Closure2>{
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
    return neutron::pipeline_result<Closure1, Closure2>{
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2)
    };
}
